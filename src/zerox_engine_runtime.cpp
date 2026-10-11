#include <filesystem>
#include <stdexcept>

#include <glad/glad.h>
#include <GL/glx.h>
#include <GL/glext.h>

#include <renderer.hpp>
#include <shader.h>
#include <texture.h>

#include "render_command_generator.h"
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

        return platform::copyFile(gameLibraryPath.c_str(), destinationPath.c_str());
    }
}

namespace ZeroX
{
    EngineRuntime::EngineRuntime(const WindowConfig& config)
    {
        if (!glfwInit())
        {
            const char* description{ nullptr };
            int code = glfwGetError(&description);
            throw std::runtime_error("Failed to intialize GLFW library. Error code: " + std::to_string(code) + " - " + std::string(description));
        }

        m_window = std::make_unique<GLFWWindow>(config);
    }

    EngineRuntime::~EngineRuntime()
    {
        m_window.reset();

        glfwTerminate();
    }

    void EngineRuntime::run(const GameConfig& gameConfig, const AudioConfig& audioConfig)
    {
        renderer::FrameBuffer renderFrameBuffer;
        renderFrameBuffer.allocate(64);

        m_window->setResizeCallback([&](int width, int height) -> void {
            renderer::Frame frame;
            renderer::Command setAspectRatioCommand{ .type = renderer::CommandType::SET_CAMERA_ASPECT_RATIO };
            std::memcpy(setAspectRatioCommand.data, &width, sizeof(int));
            std::memcpy(setAspectRatioCommand.data + sizeof(int), &height, sizeof(int));
            frame.renderCommands.push_back(setAspectRatioCommand);
            while (!renderFrameBuffer.push(frame))
            {

            }
        });

        m_window->unbindGraphicsContext();
        m_rendererThread = std::thread(&EngineRuntime::render, this, &renderFrameBuffer);

        m_gameLibraryWatcher    = std::make_unique<GameLibraryWatcher>();
        m_gameLibrary           = std::make_unique<GameLibrary>();

        BumpAllocator<16> gameAllocator;
        gameAllocator.allocate(4096);

        uint64_t libraryChangeCounter = 0;

        if (!initializeGameLibrary(gameConfig))
        {
            LOG_WARN << "Failed to initialize game library";
        }
        else
        {
            LOG_INFO << "Successfully initialized game libary. Setting change count to: " << libraryChangeCounter;
        }

        if (!m_gameLibrary->getFunctions().initGame(&gameAllocator))
        {
            LOG_WARN << "Failed to initialize game library memory";
        }
        else
        {
            LOG_INFO << "Successfully initialized game library memory";
        }

        m_audioPlayer = std::make_unique<MiniAudioPlayer>();
        if (!m_audioPlayer->initialize(audioConfig))
            throw std::runtime_error("Failed to initialize audio player");

        LOG_INFO << "Successfully initialized audio player";

        uint64_t frameIndex{ 0 };

        Time time;

        while (!m_window->shouldClose())
        {
            m_window->processEvents();

            // reload game library if necessary

            const uint64_t watchCounter = m_gameLibraryWatcher->getChangeCounter();

            if (libraryChangeCounter != watchCounter)
            {
                if (!reloadGameLibrary(gameConfig, watchCounter))
                    LOG_WARN << "Failed to re-load game library file: " << gameConfig.gameLibraryFile;

                LOG_INFO << "Successfully re-loaded game libary. Setting change count to: " << watchCounter;

                libraryChangeCounter = watchCounter;
            }

            if (!m_gameLibrary->isLoaded() || !m_gameLibrary->isValid())
            {
                continue;
            }

            // update input

            // TODO

            // wait for render thread

            const uint64_t renderFrameIndex = m_renderFrameIndex.load(std::memory_order_acquire);

            const uint64_t maxQueuedFrames = gameConfig.maxQueuedFrames;
            if (frameIndex >= maxQueuedFrames)
            {
                int64_t difference = frameIndex - renderFrameIndex;

                if (difference >= maxQueuedFrames)
                {
                    m_renderFrameIndex.wait(renderFrameIndex);
                }
            }

            // Sample delta time

            const double frameDeltaTime = m_frameDeltaTime.load(std::memory_order_acquire);
            const double alpha          = 2.0 / 61;
            time.smoothDeltaTime        = ( frameDeltaTime * alpha ) + ( time.smoothDeltaTime * (1.0 - alpha) );

            // update game

            m_gameLibrary->getFunctions().updateGame(&gameAllocator, frameDeltaTime);

            // Submit render commands

            ZeroXGame::GameMemory gameMemory = ZeroXGame::readGameMemory(&gameAllocator);

            renderer::Frame frame;

            const auto players = gameMemory.players;

            const size_t spriteIndex    = 0;
            size_t spriteBatchIndex     = 0;

            for (size_t i = 0; i < players.size(); ++i)
            {
                ZeroXGame::GameEntity* playerEntity = players.data() + i;

                const size_t entityIndex = spriteBatchIndex + i;

                frame.renderCommands.push_back(generateRenderCommand(playerEntity->transform, spriteIndex, entityIndex));
                frame.renderCommands.push_back(generateRenderCommand(playerEntity->material, spriteIndex, entityIndex));
            }

            spriteBatchIndex += players.size();

            const auto enemies = gameMemory.enemies;

            for (size_t i = 0; i < enemies.size(); ++i)
            {
                ZeroXGame::GameEntity* enemyEntity = enemies.data() + i;

                const size_t entityIndex = spriteBatchIndex + i;

                frame.renderCommands.push_back(generateRenderCommand(enemyEntity->transform, spriteIndex, entityIndex));
                frame.renderCommands.push_back(generateRenderCommand(enemyEntity->material, spriteIndex, entityIndex));
            }

            spriteBatchIndex += enemies.size();

            while(!m_window->shouldClose() && !renderFrameBuffer.push(frame))
            {
                
            }

            globalEngineLogger->processMessages();

            ++frameIndex;
        }

        m_gameLibraryWatcher->stopWatching();

        stopRendering();
    }

    void EngineRuntime::stopRendering()
    {
        m_runRenderer.store(false, std::memory_order_release);

        if (m_rendererThread.joinable())
        {
            m_rendererThread.join();
        }
    }

    bool EngineRuntime::startWatchingGameLibrary(const GameConfig& config)
    {
        const std::filesystem::path gameDirectoryPath(config.gameLibraryDir);
        if (!std::filesystem::is_directory(gameDirectoryPath))
        {
            LOG_ERROR << "Failed to start watching game library: " << gameDirectoryPath.string() << " is not a directory";
            return false;
        }

        return m_gameLibraryWatcher->startWatching(gameDirectoryPath.c_str(), config.gameLibraryFile);
    }

    bool EngineRuntime::initializeGameLibrary(const GameConfig& config)
    {
        if (!startWatchingGameLibrary(config))
        {
            LOG_ERROR << "Failed to initialize game library: Could not start file watcher";
            return false;
        }

        std::filesystem::path gameLibraryPath(config.gameLibraryDir);
        gameLibraryPath /= config.gameLibraryFile;

        if (!std::filesystem::exists(gameLibraryPath))
        {
            LOG_ERROR << "Failed to initialize game library: Is not a file: " << gameLibraryPath.c_str();
            return false;
        }

        return m_gameLibrary->load(gameLibraryPath.c_str());
    }

    bool EngineRuntime::reloadGameLibrary(const GameConfig& config, uint64_t changeCounter)
    {
        const std::filesystem::path gameDirectoryPath(config.gameLibraryDir);
        const std::filesystem::path gameLibraryPath = gameDirectoryPath / config.gameLibraryFile;
 
        if (platform::getFileSize(gameLibraryPath.c_str()) == 0)
        {
            LOG_ERROR << "Failed to reload game library: File size is 0";
            return false;
        }

        std::filesystem::path destinationPath;
        if (!::copyGameLibrary(gameDirectoryPath, gameLibraryPath, destinationPath, changeCounter))
        {
            LOG_ERROR << "Failed to reload game library: Failed to copy file from " << gameLibraryPath.c_str() << " to " << destinationPath.c_str();
            return false;
        }

        return m_gameLibrary->load(destinationPath.c_str());
    }

    void EngineRuntime::render(renderer::FrameBuffer* frameBuffer)
    {
        m_window->bindGraphicsContext();

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            LOG_ERROR << "Failed to initialize rendering logic. GLAD GL function loading failed";
            return;
        }

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
            .fov    = 80.0f,
            .aspect = 1920.0f / 1080.0f,
            .near   = 1.0f,
            .far    = 100.0f
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

        renderer::RenderBatchConfig renderBatchConfigs[1] = {
            {
                .batchIndex = 0,
                .shaderProgramIndex = 1,
                .vertexArrayIndex = 0,
                .shaderLocationsIndex = 1,
                .meshIndex = 0
            }
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
            .renderEntityCounts = &entityCount,
            .shaders = shaderList,
            .cameraEye = &cameraEye,
            .cameraFrustum = &cameraFrustum,
            .cameraUniformBuffer = "CameraMatrices",
            .cameraUniformNames = cameraUniformNames,
            .shaderCompileSteps = shaderCompileSteps,
            .shaderLocationsLinks = shaderLocationsLinks,
            .renderBatchConfigs = renderBatchConfigs,
            .meshCount = 1,
            .bufferCount = 3,
            .vertexArrayCount = 1,
            .textureCount = 1,
            .shaderCount = 4,
            .shaderProgramCount = 2,
            .locationsDescriptorCount = 2,
            .renderBatchCount = 1,
            .compileStepCount = 2,
            .locationsLinkCount = 2,
            .batchConfigCount = 1
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
                renderer::processFrame(mutableGraphicsMemory, &renderFrame);
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