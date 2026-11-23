#include <filesystem>
#include <stdexcept>

#include <glad/gl.h>

#include <render_buffer.h>
#include <renderer.hpp>

#include <zerox_engine_functions.h>
#include <zerox_audio.h>

#include "config/zerox_config_loader.h"
#include "zerox_command_builder.h"
#include "zerox_command_pool.h"
#include "zerox_engine_input.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"
#include "zerox_glfw_window.h"
#include "zerox_memory_map.h"
#include "zerox_miniaudio_player.h"
#include "zerox_time.h"

#ifdef _WIN32
#include "zerox_d3d11_device.h"
#include "zerox_wgl_interop.h"
#endif

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
    EngineRuntime::EngineRuntime() : m_context(std::make_unique<EngineContext>()), m_renderCommandPool(std::make_unique<CommandPool>()), m_audioCommandPool(std::make_unique<CommandPool>()) { }

    EngineRuntime::~EngineRuntime()
    {
        stopPlayingAudio();
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
        LOG_INFO << "Successfully intialized Window instance. Starting rendering thread...";

        m_renderCommandPool->allocate(64);

        auto graphicsPair = graphicsFuture.get();

        m_rendererThread = std::thread(
            &EngineRuntime::render,
            this, graphicsPair.first, window, m_renderCommandPool.get());

        IAudioPlayer* audioPlayer = audioPlayerFuture.get();
        LOG_INFO << "Successfully intialized audio player. Starting audio thread...";

        m_audioCommandPool->allocate(16);
        m_audioThread = std::thread(
            &EngineRuntime::processAudio,
            this, audioPlayer, m_audioCommandPool.get());

        GameLibrary* gameLibrary = gameLibraryFuture.get();

        BumpAllocator<16> gameAllocator;
        gameAllocator.allocate(16384);

        using ZeroXBridge::EngineHandle;

        ZeroXBridge::EngineFunctions engineFunctions{
            .handle                 = this,
            .getFileSize            = &platform::getFileSize,
            .readFile               = &platform::readFile,
            .allocateTexture        = [](EngineHandle handle, const char* file, const ZeroXBridge::Texture* texture) -> void {  },
            .allocateTextureArray   = [](EngineHandle handle, const char* const* files, const ZeroXBridge::Texture* texture, size_t count) -> void { },
            .allocateAudio          = [](EngineHandle handle, const char* path, const ZeroXBridge::AudioSound* sound) -> void { reinterpret_cast<EngineRuntime*>(handle)->allocateAudioFile({ .path = path, .sound = *sound }); },
            .playAudio              = [](EngineHandle handle, size_t index) -> void { }
        };

        ZeroXBridge::GameContext gameContext = gameLibrary->getFunctions().initGame(&gameAllocator, &engineFunctions);
        if (!gameContext.input)
        {
            LOG_WARN << "Failed to initialize game library";
        }
        else
        {
            LOG_INFO << "Successfully intialized game library. Monitoring file updates";
        }

        if (!gameLibrary->getFunctions().loadLevel(&gameContext, &gameAllocator, 0))
        {
            LOG_WARN << "Failed to load level at index 0";
        }

        window->setResizeCallback([&](int width, int height) -> void {
            if (width <= 0 || height <= 0) { return; }

#ifdef _WIN32
            m_resizeWidth.store(static_cast<UINT>(width), std::memory_order_release);
            m_resizeHeight.store(static_cast<UINT>(height), std::memory_order_release);
            m_resizeRequested.store(true, std::memory_order_release);
#else
            renderer::Frame frame;
            renderer::Command command{ .type = renderer::CommandType::SET_CLIENT_AREA };
            renderer::SetClientAreaCommand clientAreaCommand = {
                .width  = static_cast<uint64_t>(width),
                .height = static_cast<uint64_t>(height)
            };
            std::memcpy(command.data, &clientAreaCommand, sizeof(renderer::SetClientAreaCommand));
            frame.renderCommands.push_back(command);
            while (!renderFrameBuffer.push(frame)) { }
#endif
        });

        monitorJoystickConnections();

        uint64_t frameIndex{ 0 };

        const MemoryMap& graphicsMemoryMap = graphicsPair.second;

        GameLibraryWatcher* gameLibraryWatcher = gameLibraryWatcherFuture.get();
        uint64_t libraryChangeCounter{ 0 };

        while (!window->shouldClose())
        {
            window->processEvents();

            updateInput(gameContext.input);

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

                    gameLibrary->getFunctions().setEngineFunctions(&engineFunctions);
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

            CommandBufferBuilder bufferBuilder;
            bufferBuilder.processRenderDescriptor(reinterpret_cast<const std::byte*>(gameAllocator.peek()), graphicsMemoryMap, renderDescriptor);

            while (!m_renderCommandPool->push(bufferBuilder.getBuffer()))
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

    void EngineRuntime::stopPlayingAudio()
    {
        while (!m_audioCommandPool->push({ { EngineCommand{ .type = CommandType::SHUTDOWN_AUDIO } } }));
        m_runAudio.store(false, std::memory_order_release);
        m_audioCommandSemaphore.release();

        if (m_audioThread.joinable())
        {
            m_audioThread.join();
        }
    }

    void EngineRuntime::allocateAudioFile(const AudioFile& file)
    {
        EngineCommand allocateAudioCommand{ .type = CommandType::ALLOCATE_AUDIO };

        std::vector<std::byte>& paramData = allocateAudioCommand.paramData;

        const std::string& path = file.path;

        paramData.resize(sizeof(ZeroXBridge::AudioSound) +path.size() + 1);

        std::memcpy(paramData.data(), &file.sound, sizeof(ZeroXBridge::AudioSound));
        std::memcpy(paramData.data() + sizeof(ZeroXBridge::AudioSound), path.data(), paramData.size() - 1);

        while (!m_audioCommandPool->push({ { allocateAudioCommand } }));

        m_audioCommandSemaphore.release();
    }

    void EngineRuntime::render(const GraphicsConfig& config, IWindow* window, CommandPool* commandPool)
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

#ifdef _WIN32
        glfwSwapInterval(0);

        IWindow::Size windowSize = window->getSize();

        const auto* glfwWindow = dynamic_cast<const GLFWWindow*>(window);

        HWND nativeWindowHandle = glfwWindow->getNativeWin32Handle();

        std::unique_ptr<D3D11Device> d3d11Device = std::make_unique<D3D11Device>(
            nativeWindowHandle,
            static_cast<UINT>(windowSize.width),
            static_cast<UINT>(windowSize.height));

        HDC hdc = GetDC(nativeWindowHandle);
        if (!gladLoadWGL(hdc, glfwGetProcAddress))
        {
            LOG_ERROR << "Failed to initialize GLAD WGL logic";
            return;
        }

        renderer::RenderBuffer renderBuffer;
        renderer::initializeRenderBuffer(&renderBuffer);

        ZeroXBridge::Texture renderBufferParams = {
            .wrapModeS = ZeroXBridge::TextureWrapMode::CLAMP_TO_EDGE,
            .wrapModeT = ZeroXBridge::TextureWrapMode::CLAMP_TO_EDGE,
            .minFilter = ZeroXBridge::TextureFilter::LINEAR,
            .magFilter = ZeroXBridge::TextureFilter::LINEAR
        };

        renderer::setTextureParameters(renderBuffer.textureHandle, GL_TEXTURE_2D, &renderBufferParams, true);

        WGLInterop interopLayer(renderBuffer, d3d11Device->getDevice().Get(), d3d11Device->getClientSurface().Get());

        renderer::bindRenderBuffer(&renderBuffer);
#endif
        constexpr size_t shaderCount{ 10 };
        std::vector<std::vector<char>> shaderFileContents;
        shaderFileContents.reserve(shaderCount);

        auto loadShaderFile = [&shaderFileContents](const char* path) -> void {
            const size_t fileSize = platform::getFileSize(path);
            if (fileSize == 0)
            {
                return;
            }

            std::vector<char> contents(fileSize + 1, '\0');
            if (!platform::readFile(path, contents.data(), fileSize))
            {
                return;
            }

            shaderFileContents.push_back(std::move(contents));
        };

        // TODO(Karim): Could be async!

        loadShaderFile("./shaders/unlit_color/vertex.slh");
        loadShaderFile("./shaders/unlit_color/fragment.slh");
        loadShaderFile("./shaders/background/vertex.slh");
        loadShaderFile("./shaders/background/fragment.slh");
        loadShaderFile("./shaders/sprite/vertex.slh");
        loadShaderFile("./shaders/sprite/fragment.slh");
        loadShaderFile("./shaders/tiled_sprite/vertex.slh");
        loadShaderFile("./shaders/tiled_sprite/fragment.slh");
        loadShaderFile("./shaders/collider/vertex.slh");
        loadShaderFile("./shaders/collider/fragment.slh");

        std::vector<renderer::ShaderLoadInfo> shaders;
        shaders.reserve(10);
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::VERTEX, .content = shaderFileContents[0].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::FRAGMENT, .content = shaderFileContents[1].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::VERTEX, .content = shaderFileContents[2].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::FRAGMENT, .content = shaderFileContents[3].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::VERTEX, .content = shaderFileContents[4].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::FRAGMENT, .content = shaderFileContents[5].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::VERTEX, .content = shaderFileContents[6].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::FRAGMENT, .content = shaderFileContents[7].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::VERTEX, .content = shaderFileContents[8].data() });
        shaders.push_back(renderer::ShaderLoadInfo{ .type = renderer::Shader::Type::FRAGMENT, .content = shaderFileContents[9].data() });

        std::vector<std::vector<size_t>> compileIndices;
        compileIndices.reserve(5);
        compileIndices.push_back({ 0, 1 });
        compileIndices.push_back({ 2, 3 });
        compileIndices.push_back({ 4, 5 });
        compileIndices.push_back({ 6, 7 });
        compileIndices.push_back({ 8, 9 });

        std::vector<renderer::ShaderCompileStep> shaderCompileSteps;
        shaderCompileSteps.reserve(compileIndices.size());
        shaderCompileSteps.push_back(renderer::ShaderCompileStep{ .shaderIndices = compileIndices[0].data(), .shaderCount = compileIndices[0].size() });
        shaderCompileSteps.push_back(renderer::ShaderCompileStep{ .shaderIndices = compileIndices[1].data(), .shaderCount = compileIndices[1].size() });
        shaderCompileSteps.push_back(renderer::ShaderCompileStep{ .shaderIndices = compileIndices[2].data(), .shaderCount = compileIndices[2].size() });
        shaderCompileSteps.push_back(renderer::ShaderCompileStep{ .shaderIndices = compileIndices[3].data(), .shaderCount = compileIndices[3].size() });
        shaderCompileSteps.push_back(renderer::ShaderCompileStep{ .shaderIndices = compileIndices[4].data(), .shaderCount = compileIndices[4].size() });

        const std::vector<MeshConfig>& meshConfigs = config.getMeshes();

        // TODO(Karim): This shouldn't be necessary:
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

        renderer::GraphicsConfig graphicsConfig = {
            .meshes                     = meshes.data(),
            .textures                   = config.getTextures().data(),
            .shaders                    = shaders.data(),
            .cameraEye                  = &config.getCameraEye(),
            .cameraFrustum              = &config.getCameraFrustum(),
            .cameraUniformBuffer        = config.getCameraUniformBuffer().data(),
            .cameraUniformNames         = config.getCameraUniformNames().data(),
            .shaderCompileSteps         = shaderCompileSteps.data(),
            .textureArrays              = config.getTextureArrays().data(),
            .meshCount                  = meshes.size(),
            .bufferCount                = 5,
            .vertexArrayCount           = config.getVertexArrayCount(),
            .textureCount               = config.getTextures().size(),
            .textureArrayCount          = config.getTextureArrays().size(),
            .shaderCount                = shaders.size(),
            .shaderProgramCount         = shaderCompileSteps.size(),
            .compileStepCount           = shaderCompileSteps.size()
        };

        renderer::Allocator renderAllocator;
        renderer::allocateGraphicsResources(&renderAllocator, &graphicsConfig);
        renderer::initializeGraphicsResources(&renderAllocator, &graphicsConfig);
        renderer::MutableGraphicsMemory mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
        renderer::initializeGraphicsState();

        const IWindow::Size size = window->getSize();
        renderer::setCameraAspectRatio(mutableGraphicsMemory, size.width, size.height);

        uint64_t frameIndex{ 0 };
        auto startTimePoint = std::chrono::steady_clock::now();

        GLint megaTextureId{ 0 };
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &megaTextureId);

        while (m_runRenderer.load(std::memory_order_acquire))
        {
#ifdef _WIN32
            if (m_resizeRequested.load(std::memory_order_acquire))
            {
                const UINT newWidth     = m_resizeWidth.load(std::memory_order_acquire);
                const UINT newHeight    = m_resizeHeight.load(std::memory_order_acquire);

                interopLayer.release();
                renderer::destroyRenderBuffer(&renderBuffer);

                d3d11Device->resizeBuffers(newWidth, newHeight);

                renderer::initializeRenderBuffer(&renderBuffer);
                renderer::setTextureParameters(renderBuffer.textureHandle, GL_TEXTURE_2D, &renderBufferParams, true);

                interopLayer.initialize(renderBuffer, d3d11Device->getDevice().Get(), d3d11Device->getClientSurface().Get());
                renderer::bindRenderBuffer(&renderBuffer);

                glViewport(0, 0, static_cast<int>(newWidth), static_cast<int>(newHeight));
                renderer::setCameraAspectRatio(mutableGraphicsMemory, static_cast<int>(newWidth), static_cast<int>(newHeight));

                GLint currentTextureId{ 0 };
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTextureId);
                if (currentTextureId != megaTextureId)
                {
                    glBindTexture(GL_TEXTURE_2D, megaTextureId);
                }

                m_resizeRequested.store(false, std::memory_order_release);
            }

            wglDXLockObjectsNV(interopLayer.getWGLDXDevice(), 1, &interopLayer.getWGLSharedTexture());
            glBindFramebuffer(GL_FRAMEBUFFER, renderBuffer.frameBufferHandle);
#endif
            CommandBuffer commandBuffer;
            if (commandPool->pop(commandBuffer))
            {
                for (const auto& command : commandBuffer.commands)
                {
                    std::span<const std::byte> paramsSpan(command.paramData.data(), command.paramData.size());
                    ConstMemoryView paramsView(paramsSpan);

                    switch (command.type)
                    {
                    case CommandType::ALLOCATE_RENDER_BATCH:
                        renderer::handleAllocateBatchCommand(&renderAllocator, mutableGraphicsMemory, *paramsView.read_object<renderer::AllocateBatchCommand>(0).data());
                        mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
                        break;
                    case CommandType::UPDATE_ENTITY_TRANSFORM:
                        renderer::handleUpdateTransformCommand(&renderAllocator, mutableGraphicsMemory, *paramsView.read_object<renderer::UpdateTransformCommand>(0).data());
                        break;
                    case CommandType::UPDATE_ENTITY_MATERIAL:
                        renderer::handleUpdateMaterialCommand(&renderAllocator, mutableGraphicsMemory, *paramsView.read_object<renderer::UpdateMaterialCommand>(0).data());
                        break;
                    case CommandType::SET_TEXTURE_LAYER:
                        renderer::handleSetTextureLayerCommand(&renderAllocator, mutableGraphicsMemory, *paramsView.read_object<renderer::SetTextureLayerCommand>(0).data());
                        break;
                    case CommandType::SET_CLIENT_AREA:
                        renderer::handleSetClientAreaCommand(&renderAllocator, mutableGraphicsMemory, *paramsView.read_object<renderer::SetClientAreaCommand>(0).data());
                        break;
                    default:
                        break;
                    }
                }
            }

            renderer::ConstGraphicsMemory renderMemory = renderer::readGraphicsMemory<true>(&renderAllocator);

            const IWindow::Size clientSize = window->getSize();

            const glm::vec2 worldDimensions = renderer::getViewportWorldDimensions(
                mutableGraphicsMemory.camera.data(),
                mutableGraphicsMemory.cameraEye.data(),
                mutableGraphicsMemory.cameraFrustum.data());

            renderer::clearFrameBuffer();
            renderer::render(renderMemory, renderer::getPixelsPerUnit(clientSize.width, clientSize.height, worldDimensions));

#ifdef _WIN32
            // TODO(Karim): Explicit glFlush() call here?

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            wglDXUnlockObjectsNV(interopLayer.getWGLDXDevice(), 1, &interopLayer.getWGLSharedTexture());

            d3d11Device->copySurfaceToBackBuffer(static_cast<UINT>(clientSize.width), static_cast<UINT>(clientSize.height));
            d3d11Device->getSwapChain()->Present(1, 0);
#else
            window->swapBuffers();
#endif
            auto currentTimePoint       = std::chrono::steady_clock::now();
            int64_t microSecondsElapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTimePoint - startTimePoint).count();

            m_frameDeltaTime.store(microSecondsElapsed * 0.000001, std::memory_order_release);
            
            startTimePoint = currentTimePoint;

            m_renderFrameIndex.store(frameIndex, std::memory_order_release);
            m_renderFrameIndex.notify_one();

            ++frameIndex;
        }

        renderer::freeGraphicsResources(mutableGraphicsMemory);

#ifdef _WIN32
        renderer::destroyRenderBuffer(&renderBuffer);

        ReleaseDC(nativeWindowHandle, hdc);
#endif
    }

    void EngineRuntime::processAudio(IAudioPlayer* player, CommandPool* commandPool)
    {
        while (m_runAudio.load(std::memory_order_acquire))
        {
            m_audioCommandSemaphore.acquire();

            CommandBuffer commandBuffer;
            if (commandPool->pop(commandBuffer))
            {
                for (const auto& command : commandBuffer.commands)
                {
                    switch (command.type)
                    {
                    case CommandType::ALLOCATE_AUDIO:
                    {
                        AudioFile file{
                            .path   = std::string(reinterpret_cast<const char*>(command.paramData.data() + sizeof(ZeroXBridge::AudioSound))),
                            .sound  = *reinterpret_cast<const ZeroXBridge::AudioSound*>(command.paramData.data())
                        };

                        player->loadSound(file);

                        break;
                    }
                    case CommandType::SHUTDOWN_AUDIO:
                        // TODO(Karim): Maybe unload sounds etc..?
                        break;
                    }
                }
            }
        }
    }
}