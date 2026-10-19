#include <filesystem>
#include <stdexcept>

#include <glad/gl.h>

#include <renderer.hpp>
#include <shader.h>
#include <texture.h>

#include "render_command_generator.h"
#include "zerox_engine_input.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"
#include "zerox_glfw_window.h"
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

    renderer::Frame buildRenderFrame(BumpAllocator<16>* gameAllocator, const ZeroX::RenderDescriptor& descriptor)
    {
        std::byte* gameMemory = static_cast<std::byte*>(gameAllocator->peek());

        renderer::Frame result;

        auto* entitySectionData = descriptor.entitySectionData;
        MemoryCursor<16> cursor;

        for (uint64_t i = 0; i < descriptor.sectionCount; ++i)
        {
            auto* entitySection = reinterpret_cast<ZeroX::EntitySection*>(entitySectionData + cursor.getOffset());

            ZeroX::EntitySectionFlags& sectionFlags = entitySection->flags;

            // TODO(Karim): This double switch needs refactoring. Perhaps write a function to pre-populate indices based on desired mappings?
            
            const uint64_t descriptorCount = entitySection->descriptorCount;

            switch (entitySection->type)
            {
            case ZeroX::EntitySectionType::SPRITE:
            {
                if ( ( sectionFlags & ZeroX::EntitySectionFlags::ALLOCATE_BATCH ) == ZeroX::EntitySectionFlags::ALLOCATE_BATCH )
                {
                    renderer::Command command{ .type = renderer::CommandType::ALLOCATE_RENDER_BATCH };

                    // TODO(Karim): Maybe perform a lookup here to avoid magic numbers?
                    const size_t shaderProgramIndex = 1;
                    const size_t vertexArrayIndex   = 0;
                    const size_t locationsIndex     = 1;
                    const size_t meshIndex          = 0;

                    std::memcpy(command.data, &shaderProgramIndex, sizeof(size_t));
                    std::memcpy(command.data + sizeof(size_t), &vertexArrayIndex, sizeof(size_t));
                    std::memcpy(command.data + 2 * sizeof(size_t), &locationsIndex, sizeof(size_t));
                    std::memcpy(command.data + 3 * sizeof(size_t), &meshIndex, sizeof(size_t));
                    std::memcpy(command.data + 4 * sizeof(size_t), &descriptorCount, sizeof(size_t));

                    result.renderCommands.push_back(command);

                    sectionFlags &= ~ZeroX::EntitySectionFlags::ALLOCATE_BATCH;
                }

                break;
            }
            default:
                break;
            }

            cursor.step<ZeroX::EntitySection>();

            for (uint64_t j = 0; j < descriptorCount; ++j)
            {
                const auto* fetchDescriptor = entitySection->fetchDescriptors + j;

                const renderer::RenderEntity* renderEntity = reinterpret_cast<const renderer::RenderEntity*>(gameMemory + fetchDescriptor->offset + fetchDescriptor->stride);

                switch (entitySection->type)
                {
                case ZeroX::EntitySectionType::SPRITE:
                {
                    result.renderCommands.push_back(ZeroX::generateRenderCommand(renderEntity->transform, 0, j));
                    result.renderCommands.push_back(ZeroX::generateRenderCommand(renderEntity->material, 0, j));
                    break;
                }
                default:
                    break;
                }
            }

            cursor.step_array<ZeroX::EntityFetchDescriptor>(descriptorCount);
        }

        return result;
    }
}

namespace ZeroX
{
    EngineRuntime::EngineRuntime()
    {
        if (!glfwInit())
        {
            const char* description{ nullptr };
            int code = glfwGetError(&description);
            throw std::runtime_error("Failed to intialize GLFW library. Error code: " + std::to_string(code) + " - " + std::string(description));
        }

        m_contextLoader = std::make_unique<ContextLoader>();
    }

    EngineRuntime::~EngineRuntime()
    {
        m_contextLoader.reset();
        m_window.reset();

        glfwTerminate();
    }

    void EngineRuntime::load(const char* configPath)
    {
        auto windowConfigFuture         = m_contextLoader->getWindowConfigFuture();
        auto audioPlayerFuture          = m_contextLoader->getAudioPlayerFuture();
        auto gameLibraryFuture          = m_contextLoader->getGameLibraryFuture();
        auto gameLibraryWatcherFuture   = m_contextLoader->getGameLibraryWatcherFuture();

        m_contextLoader->load(configPath);

        renderer::FrameBuffer renderFrameBuffer;
        renderFrameBuffer.allocate(64);

        m_window = std::make_unique<GLFWWindow>(windowConfigFuture.get());
        LOG_INFO << "Successfully intialized Window instance. Starting rendering thread";
        m_rendererThread = std::thread(&EngineRuntime::render, this, &renderFrameBuffer);

        // TODO(Karim): Audio stuff...
        audioPlayerFuture.get();
        LOG_INFO << "Successfully initialized audio player";

        GameLibrary* gameLibrary = gameLibraryFuture.get();
        LOG_INFO << "Successfully intialized game library. Proceeding to initialize allocator";

        BumpAllocator<16> gameAllocator;
        gameAllocator.allocate(4096);

        auto* input = gameAllocator.requestMemory<Input>();

        if (!gameLibrary->getFunctions().initGame(&gameAllocator))
        {
            throw std::runtime_error("Game library failed to initialize allocator memory");
        }

        LOG_INFO << "Game library successfully initialized allocator memory. Starting file watch";

        uint64_t libraryWatchCounter{ 0 };
        GameLibraryWatcher* gameLibraryWatcher = gameLibraryWatcherFuture.get();

        m_window->setResizeCallback([&](int width, int height) -> void {
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

        uint64_t frameIndex{ 0 };
        Time time = { };

        while (!m_window->shouldClose())
        {
            m_window->processEvents();

            updateInput(input);

            const uint64_t watchCounter = gameLibraryWatcher->getChangeCounter();

            if (libraryWatchCounter != watchCounter)
            {
                const platform::FileWriteEvent& libraryWriteEvent = gameLibraryWatcher->getFileWriteEvent();

                if (!reloadGameLibrary(libraryWriteEvent.parentDirectory.c_str(), libraryWriteEvent.targetFilename.c_str(), watchCounter))
                {
                    LOG_WARN << "Failed to re-load game library file: " << libraryWriteEvent.targetFilename;
                }
                else
                {
                    LOG_INFO << "Successfully re-loaded game libary. Setting change count to: " << watchCounter;
                }

                libraryWatchCounter = watchCounter;
            }

            if (!gameLibrary->isLoaded() || !gameLibrary->isValid())
            {
                globalEngineLogger->processMessages();
                continue;
            }

            const uint64_t renderFrameIndex = m_renderFrameIndex.load(std::memory_order_acquire);

            const uint64_t maxQueuedFrames = 4;
            if (frameIndex >= maxQueuedFrames)
            {
                int64_t difference = frameIndex - renderFrameIndex;

                if (difference >= maxQueuedFrames)
                {
                    m_renderFrameIndex.wait(renderFrameIndex);
                }
            }

            const double frameDeltaTime = m_frameDeltaTime.load(std::memory_order_acquire);
            const double alpha          = 2.0 / 61;
            time.smoothDeltaTime        = ( frameDeltaTime * alpha ) + ( time.smoothDeltaTime * (1.0 - alpha) );

            gameLibrary->getFunctions().updateGame(&gameAllocator, time.smoothDeltaTime);

            const RenderDescriptor renderDescriptor = gameLibrary->getFunctions().initRenderDescriptor(&gameAllocator);
            const renderer::Frame renderFrame       = ::buildRenderFrame(&gameAllocator, renderDescriptor);
            
            while (!renderFrameBuffer.push(renderFrame))
            {
            
            }

            globalEngineLogger->processMessages();

            ++frameIndex;
        }

        gameLibraryWatcher->stopWatching();

        m_runRenderer.store(false, std::memory_order_release);

        if (m_rendererThread.joinable())
        {
            m_rendererThread.join();
        }
    }

    bool EngineRuntime::reloadGameLibrary(const char* parentDirectory, const char* filename, uint64_t counter)
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

        return m_contextLoader->getEngineContext()->gameLibrary->load(destinationPath.string().c_str());
    }

    void EngineRuntime::render(renderer::FrameBuffer* frameBuffer)
    {
        m_window->bindGraphicsContext();

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

        renderer::Vertex quadVertices[4] = {
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
            { { -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
            { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
        };

        unsigned int quadTriangles[6] = { 0, 2, 1, 0, 3, 2 };

        renderer::Mesh meshes[1] = { { quadVertices, quadTriangles, 4, 6 } };

        renderer::Shader shaderList[4] = {
            { "./shaders/unlit_color/vertex.slh", renderer::Shader::Type::VERTEX },
            { "./shaders/unlit_color/fragment.slh", renderer::Shader::Type::FRAGMENT },
            { "./shaders/sprite/vertex.slh", renderer::Shader::Type::VERTEX },
            { "./shaders/sprite/fragment.slh", renderer::Shader::Type::FRAGMENT },
        };

        renderer::Eye cameraEye = {
            .position   = glm::vec3{ 0.0f, 0.0f, 10.0f },
            .target	    = glm::vec3{ 0.0f, 0.0f, 0.0f },
            .up		    = glm::vec3{ 0.0f, 1.0f, 0.0f }
        };

        renderer::Frustum cameraFrustum = {
            .fov        = 80.0f,
            .aspect     = 1920.0f / 1080.0f,
            .nearPlane  = 1.0f,
            .farPlane   = 100.0f
        };

        const char* cameraUniformNames[4] = {
            "cameraProjection",
            "cameraLocalToWorld",
            "cameraLocalRotation",
            "cameraView"
        };

        const size_t entityCount = 5;

        size_t unlitColorIndices[2] = { 0, 1 };
        size_t spriteIndices[2] = { 2, 3 };

        renderer::ShaderCompileStep shaderCompileSteps[2] = {
            { .shaderIndices = unlitColorIndices, .shaderCount = 2, .programIndex = 0 },
            { .shaderIndices = spriteIndices, .shaderCount = 2, .programIndex = 1 },
        };

        renderer::ShaderLocationsLink shaderLocationsLinks[2] = {
            { .locationsIndex = 0, .shaderProgramIndex = 0 },
            { .locationsIndex = 1, .shaderProgramIndex = 1 },
        };

        renderer::Texture textures[1] = {
            {
                .path = "./textures/the_mega_texture.png",
                .wrapModeS = renderer::WrapMode::CLAMP_TO_EDGE,
                .wrapModeT = renderer::WrapMode::CLAMP_TO_EDGE,
                .minFilter = renderer::Filter::NEAREST,
                .magFilter = renderer::Filter::NEAREST
            }
        };

        renderer::GraphicsConfig graphicsConfig = {
            .meshes = meshes,
            .textures = textures,
            .shaders = shaderList,
            .cameraEye = &cameraEye,
            .cameraFrustum = &cameraFrustum,
            .cameraUniformBuffer = "CameraMatrices",
            .cameraUniformNames = cameraUniformNames,
            .shaderCompileSteps = shaderCompileSteps,
            .shaderLocationsLinks = shaderLocationsLinks,
            .meshCount = 1,
            .bufferCount = 3,
            .vertexArrayCount = 1,
            .textureCount = 1,
            .shaderCount = 4,
            .shaderProgramCount = 2,
            .locationsDescriptorCount = 2,
            .compileStepCount = 2,
            .locationsLinkCount = 2
        };

        renderer::Allocator renderAllocator;
        renderer::allocateGraphicsResources(&renderAllocator, &graphicsConfig);
        renderer::MutableGraphicsMemory mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
        renderer::initializeGraphicsResources(mutableGraphicsMemory, &graphicsConfig);
        renderer::initializeGraphicsState();

        const IWindow::Size size = m_window->getSize();
        renderer::setCameraAspectRatio(mutableGraphicsMemory, size.width, size.height);

        uint64_t frameIndex{ 0 };
        auto startTimePoint = std::chrono::steady_clock::now();

        while (m_runRenderer.load(std::memory_order_acquire))
        {
            renderer::Frame renderFrame;
            if (frameBuffer->pop(renderFrame))
            {
                renderer::processFrame(&renderAllocator, mutableGraphicsMemory, &renderFrame);
            }

            const IWindow::Size clientSize = m_window->getSize();

            const glm::vec2 worldDimensions = renderer::getViewportWorldDimensions(
                mutableGraphicsMemory.camera.data(),
                mutableGraphicsMemory.cameraEye.data(),
                mutableGraphicsMemory.cameraFrustum.data());

            renderer::clearFrameBuffer();
            renderer::render(mutableGraphicsMemory, renderer::getPixelsPerUnit(clientSize.width, clientSize.height, worldDimensions));
            m_window->swapBuffers();

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