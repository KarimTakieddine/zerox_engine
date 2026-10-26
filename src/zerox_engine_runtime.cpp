#include <filesystem>
#include <stdexcept>

#include <glad/gl.h>

#include <texture_array.h>

#include "config/zerox_config_loader.h"
#include "zerox_engine_input.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"
#include "zerox_frame_builder.h"
#include "zerox_glfw_window.h"
#include "zerox_memory_map.h"
#include "zerox_miniaudio_player.h"
#include "zerox_time.h"

namespace
{
    bool copyGameLibrary(const std::filesystem::path gameDirectoryPath, const std::filesystem::path& gameLibraryPath, std::filesystem::path& destinationPath, uint64_t suffix)
    {
        const auto filenamePath = gameLibraryPath.filename();

        std::string filename        = filenamePath.string();
        const std::string extension = filenamePath.extension().string();

        const auto pos = filename.find(extension);
        if (pos == std::string::npos)
        {
            LOG_ERROR << "Failed to find extension in game library file";
        }

        filename.insert(pos, "_" + std::to_string(suffix));

        destinationPath = gameDirectoryPath / filename;

        return platform::copyFile(gameLibraryPath.string().c_str(), destinationPath.string().c_str());
    }

    bool reloadGameLibrary(ZeroX::GameLibrary* gameLibrary, const char* parentDirectory, const char* filename, uint64_t counter)
    {
        const std::filesystem::path gameDirectoryPath(parentDirectory);
        const std::filesystem::path gameLibraryPath = gameDirectoryPath / filename;

        if (platform::getFileSize(gameLibraryPath.string().c_str()) == 0)
        {
            LOG_ERROR << "Failed to reload game library: File size is 0";
            return false;
        }

        std::filesystem::path destinationPath;
        if (!::copyGameLibrary(gameDirectoryPath, gameLibraryPath, destinationPath, counter))
        {
            LOG_ERROR << "Failed to reload game library: Failed to copy file from " << gameLibraryPath.string().c_str() << " to " << destinationPath.string().c_str();
            return false;
        }

        return gameLibrary->load(destinationPath.string().c_str());
    }
}

namespace ZeroX
{
    EngineRuntime::EngineRuntime() : m_context(std::make_unique<EngineContext>()) { }

    EngineRuntime::~EngineRuntime()
    {
        stopRendering();
    }

    void EngineRuntime::load(const char* configPath)
    {
        AsyncLoadPromise loadPromise;

        auto graphicsFuture             = loadGraphicsConfig();
        auto windowFuture               = loadPromise.window.get_future();
        auto audioPlayerFuture          = loadPromise.audioPlayer.get_future();
        auto gameLibraryFuture          = loadPromise.gameLibrary.get_future();
        auto gameLibraryWatcherFuture   = loadPromise.gameLibraryWatcher.get_future();

        m_context->load(configPath, std::move(loadPromise));

        IWindow* window = windowFuture.get();
        LOG_INFO << "Successfully intialized Window instance. Starting rendering thread";

        renderer::FrameBuffer renderFrameBuffer;
        renderFrameBuffer.allocate(64);
        
        auto graphicsPair = graphicsFuture.get();

        m_rendererThread = std::thread(
            &EngineRuntime::render,
            this, graphicsPair.first, window, &renderFrameBuffer);

        GameLibrary* gameLibrary = gameLibraryFuture.get();

        BumpAllocator<16> gameAllocator;
        gameAllocator.allocate(4096);

        auto* input = gameAllocator.requestMemory<Input>();

        if (!gameLibrary->getFunctions().initGame(&gameAllocator))
        {
            LOG_WARN << "Failed to initialize game library";
        }
        else
        {
            LOG_INFO << "Successfully intialized game library. Monitoring file updates";
        }

        GameLibraryWatcher* gameLibraryWatcher = gameLibraryWatcherFuture.get();
        uint64_t libraryChangeCounter{ 0 };

        IAudioPlayer* audioPlayer = audioPlayerFuture.get();
        LOG_INFO << "Successfully intialized audio player";

        window->setResizeCallback([&](int width, int height) -> void {
            if (width <= 0 || height <= 0) { return; }

            renderer::Frame frame;
            renderer::Command setAspectRatioCommand{ .type = renderer::CommandType::SET_CAMERA_ASPECT_RATIO };
            std::memcpy(setAspectRatioCommand.data, &width, sizeof(int));
            std::memcpy(setAspectRatioCommand.data + sizeof(int), &height, sizeof(int));
            frame.renderCommands.push_back(setAspectRatioCommand);
            while (!renderFrameBuffer.push(frame))
            {
            
            }
        });

        monitorJoystickConnections();

        uint64_t frameIndex{ 0 };

        const MemoryMap& graphicsMemoryMap = graphicsPair.second;

        while (!window->shouldClose())
        {
            window->processEvents();

            updateInput(input);

            const uint64_t changeCounter = gameLibraryWatcher->getChangeCounter();
            if (changeCounter != libraryChangeCounter)
            {
                const platform::FileWriteEvent& fileWriteEvent = gameLibraryWatcher->getFileWriteEvent();

                if (!::reloadGameLibrary(gameLibrary, fileWriteEvent.parentDirectory.c_str(), fileWriteEvent.targetFilename.c_str(), changeCounter))
                {
                    LOG_WARN << "Failed to reload game library. Change count is: " << changeCounter;
                }
                else
                {
                    LOG_INFO << "Successfully re-loaded game library. Change count is: " << changeCounter;
                }

                libraryChangeCounter = changeCounter;
            }

            if (!gameLibrary->isLoaded() || !gameLibrary->isValid())
            {
                globalEngineLogger->processMessages();
                continue;
            }

            const uint64_t renderFrameIndex = m_renderFrameIndex.load(std::memory_order_acquire);

            const uint64_t maxQueuedFrames = 4; // TODO(Karim): Find a way to configure this
            if (frameIndex >= maxQueuedFrames)
            {
                int64_t difference = frameIndex - renderFrameIndex;

                if (difference >= maxQueuedFrames)
                {
                    m_renderFrameIndex.wait(renderFrameIndex);
                }
            }

            const double frameDeltaTime = m_frameDeltaTime.load(std::memory_order_acquire);
            /*
                const double alpha      = 2.0 / 61;
                time.smoothDeltaTime    = ( frameDeltaTime * alpha ) + ( time.smoothDeltaTime * (1.0 - alpha) );
            */
            gameLibrary->getFunctions().updateGame(&gameAllocator, frameDeltaTime);
           
            const RenderDescriptor renderDescriptor = gameLibrary->getFunctions().initRenderDescriptor(&gameAllocator);
            FrameBuilder frameBuilder;
            frameBuilder.processRenderDescriptor(reinterpret_cast<const std::byte*>(gameAllocator.peek()), graphicsMemoryMap, renderDescriptor);

            while (!renderFrameBuffer.push(frameBuilder.getFrame()))
            {

            }

            globalEngineLogger->processMessages();

            ++frameIndex;
        }
    }

    void EngineRuntime::stopRendering()
    {
        m_runRenderer.store(false, std::memory_order_release);

        if (m_rendererThread.joinable())
        {
            m_rendererThread.join();
        }
    }

    void EngineRuntime::render(const GraphicsConfig& config, IWindow* window, renderer::FrameBuffer* frameBuffer)
    {
        window->bindGraphicsContext();

        int openGLVersion = gladLoadGL(glfwGetProcAddress);
        if (openGLVersion == 0)
        {
            LOG_ERROR << "Failed to initialize rendering logic. Glad 2 GL function loading failed";
            return;
        }

        LOG_INFO
            << "Loaded OpenGL version: "
            << GLAD_VERSION_MAJOR(openGLVersion) << "."
            << GLAD_VERSION_MINOR(openGLVersion);

        const std::vector<MeshConfig>& meshConfigs = config.getMeshes();

        std::vector<renderer::Mesh> meshes;
        meshes.reserve(meshConfigs.size());
        for (const auto& meshConfig : meshConfigs)
        {
            const auto& vertices    = meshConfig.vertices;
            const auto& triangles   = meshConfig.triangles;

            meshes.emplace_back(renderer::Mesh{
                    .vertices       = const_cast<renderer::Vertex*>(vertices.data()),
                    .triangles      = const_cast<unsigned int*>(triangles.data()),
                    .vertexCount    = vertices.size(),
                    .triangleCount  = triangles.size()
                }
            );
        }

        std::vector<renderer::Shader> shaders;
        size_t shaderCount{ 0 };
        for (const auto& shaderProgramConfig : config.getShaderPrograms())
        {
            const std::vector<renderer::Shader>& shaderConfigs = shaderProgramConfig.shaders;

            shaders.insert(shaders.begin() + shaderCount, shaderConfigs.begin(), shaderConfigs.end());

            shaderCount += shaderConfigs.size();
        }

        const std::vector<ShaderCompileConfig>& shaderCompileConfigs = config.getShaderCompileSteps();
        std::vector<renderer::ShaderCompileStep> shaderCompileSteps;
        shaderCompileSteps.reserve(shaderCompileConfigs.size());
        for (const auto& compileConfig : shaderCompileConfigs)
        {
            const std::vector<size_t>& shaderIndices = compileConfig.shaderIndices;

            shaderCompileSteps.emplace_back(
                renderer::ShaderCompileStep{
                    .shaderIndices  = shaderIndices.data(),
                    .shaderCount    = shaderIndices.size(),
                    .programIndex   = compileConfig.programIndex
                }
            );
        }

        const std::vector<renderer::ShaderLocationsLink>& shaderLocationsLinks  = config.getShaderLocationsLinks();
        const std::vector<renderer::Texture>& textures                          = config.getTextures();

        std::vector<renderer::TextureArray> textureArrays;
        textureArrays.reserve(config.getTextureArrays().size());
        for (const auto& textureArray : config.getTextureArrays())
        {
            textureArrays.emplace_back(renderer::TextureArray{
                .data       = textureArray.data,
                .imageFiles = textureArray.textureFiles.data(),
                .imageCount = textureArray.textureFiles.size()
            });
        }

        renderer::GraphicsConfig graphicsConfig = {
            .meshes                     = meshes.data(),
            .textures                   = textures.data(),
            .shaders                    = shaders.data(),
            .cameraEye                  = &config.getCameraEye(),
            .cameraFrustum              = &config.getCameraFrustum(),
            .cameraUniformBuffer        = config.getCameraUniformBuffer().data(),
            .cameraUniformNames         = config.getCameraUniformNames().data(),
            .shaderCompileSteps         = shaderCompileSteps.data(),
            .shaderLocationsLinks       = shaderLocationsLinks.data(),
            .textureArrays              = textureArrays.data(),
            .meshCount                  = meshes.size(),
            .bufferCount                = 5,
            .vertexArrayCount           = config.getVertexArrayCount(),
            .textureCount               = textures.size(),
            .textureArrayCount          = textureArrays.size(),
            .shaderCount                = shaders.size(),
            .shaderProgramCount         = config.getShaderPrograms().size(),
            .locationsDescriptorCount   = shaderLocationsLinks.size(),
            .compileStepCount           = shaderCompileSteps.size(),
            .locationsLinkCount         = shaderLocationsLinks.size()
        };

        renderer::Allocator renderAllocator;
        renderer::allocateGraphicsResources(&renderAllocator, &graphicsConfig);
        renderer::MutableGraphicsMemory mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
        renderer::initializeGraphicsResources(mutableGraphicsMemory, &graphicsConfig);
        renderer::initializeGraphicsState();

        const IWindow::Size size = window->getSize();
        renderer::setCameraAspectRatio(mutableGraphicsMemory, size.width, size.height);

        uint64_t frameIndex{ 0 };
        auto startTimePoint = std::chrono::steady_clock::now();

        while (m_runRenderer.load(std::memory_order_acquire))
        {
            renderer::Frame renderFrame;
            if (frameBuffer->pop(renderFrame))
            {
                // TODO(Karim): Memory seems to be re-read a lot here. Establish what is needed!
                renderer::processFrame(&renderAllocator, mutableGraphicsMemory, &renderFrame);
            }

            const IWindow::Size clientSize = window->getSize();

            const glm::vec2 worldDimensions = renderer::getViewportWorldDimensions(
                mutableGraphicsMemory.camera.data(),
                mutableGraphicsMemory.cameraEye.data(),
                mutableGraphicsMemory.cameraFrustum.data());

            renderer::clearFrameBuffer();
            renderer::render(mutableGraphicsMemory, renderer::getPixelsPerUnit(clientSize.width, clientSize.height, worldDimensions));
            window->swapBuffers();

            auto currentTimePoint       = std::chrono::steady_clock::now();
            int64_t microSecondsElapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTimePoint - startTimePoint).count();

            m_frameDeltaTime.store(microSecondsElapsed * 0.000001, std::memory_order_release);
            
            startTimePoint = currentTimePoint;

            m_renderFrameIndex.store(frameIndex, std::memory_order_release);
            m_renderFrameIndex.notify_one();

            ++frameIndex;
        }

        renderer::freeGraphicsResources(mutableGraphicsMemory);
    }
}