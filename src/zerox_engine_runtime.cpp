#include <filesystem>

#include <glad/glad.h>
#include <GL/glx.h>
#include <GL/glext.h>

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <renderer.hpp>
#include <shader.h>
#include <texture.h>

#include "render_command_generator.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"
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

    void onWindowResize(GLFWwindow* window, int width, int height)
    {
        renderer::MutableGraphicsMemory* mutableGraphicsMemory = reinterpret_cast<renderer::MutableGraphicsMemory*>(glfwGetWindowUserPointer(window));

        renderer::setCameraAspectRatio(*mutableGraphicsMemory, width, height);

        glViewport(0, 0, width, height);
    }
}

namespace ZeroX
{
    void EngineRuntime::startGameThread(const GameConfig& config, renderer::FrameBuffer* frameBuffer)
    {
        m_gameThread = std::thread(&EngineRuntime::executeGameLoop, this, config, frameBuffer);
    }

    void EngineRuntime::startRenderThread(renderer::FrameBuffer* frameBuffer)
    {
        m_rendererThread = std::thread(&EngineRuntime::executeRenderLoop, this, frameBuffer);
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

    void EngineRuntime::executeGameLoop(const GameConfig& config, renderer::FrameBuffer* frameBuffer)
    {
        m_audioPlayer           = std::make_unique<MiniAudioPlayer>();
        m_gameLibraryWatcher    = std::make_unique<GameLibraryWatcher>();
        m_gameLibrary           = std::make_unique<GameLibrary>();

        AudioSound audioSounds[1] = {
            { "./audio/character_jump.mp3", SoundType::SOUND_EFFECT }
        };

        AudioConfig audioConfig = {
            .sounds     = audioSounds,
            .soundCount = 1
        };

        if (!m_audioPlayer->initialize(audioConfig))
        {
            LOG_ERROR << "Failed to initialize audio player";
            return;
        }

        m_audioPlayer->playSound("character_jump.mp3");

        assert(m_gameLibraryWatcher);
        assert(m_gameLibrary);

        if (!initializeGameLibrary(config))
        {
            LOG_ERROR << "Failed to initialize game library";
            return;
        }

        LOG_INFO << "Successfully initialized game library";

        uint64_t fileChangeCounter = 0;

        BumpAllocator<16> gameAllocator;
        gameAllocator.allocate(4096);
        if (!m_gameLibrary->getFunctions().initGame(&gameAllocator))
        {
            LOG_ERROR << "Failed to initialize game library memory";
            return;
        }

        LOG_INFO << "Successfully initialized game library memory";

        m_gameFrameIndex.store(0, std::memory_order_relaxed);

        while (m_runGame.load(std::memory_order_acquire))
        {
            const uint64_t watchCounter = m_gameLibraryWatcher->getChangeCounter();

            if (fileChangeCounter != watchCounter)
            {
                if (!reloadGameLibrary(config, watchCounter))
                {
                    LOG_ERROR << "Failed to reload game library. Change count is: " << fileChangeCounter;
                    continue;
                }

                LOG_INFO << "Successfully re-loaded game libary. Setting change count to: " << watchCounter;

                fileChangeCounter = watchCounter;
            }

            if (!m_gameLibrary->isLoaded() || !m_gameLibrary->isValid())
            {
                LOG_INFO << "No loaded or valid game library was found";
                continue;
            }

            Time time{
                .deltaTimeSeconds   = m_frameDeltaTime.load(std::memory_order_acquire),
                .renderFrameIndex   = m_renderFrameIndex.load(std::memory_order_acquire),
                .gameFrameIndex     = m_gameFrameIndex.load(std::memory_order_relaxed)
            };

            // Wait for frame N - maxQueuedFrames

            const uint64_t maxQueuedFrames = config.maxQueuedFrames;
            if (time.gameFrameIndex >= maxQueuedFrames)
            {
                uint64_t difference = time.gameFrameIndex - time.renderFrameIndex;

                if (difference >= maxQueuedFrames)
                {
                    m_renderFrameIndex.wait(time.renderFrameIndex);
                }
            }

            const double alpha      = 2.0 / 30;
            time.smoothDeltaTime    = ( time.smoothDeltaTime * alpha ) + ( time.deltaTimeSeconds * (1.0 - alpha) );

            m_gameLibrary->getFunctions().updateGame(&gameAllocator, time.smoothDeltaTime);

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

            while(m_runGame.load(std::memory_order_acquire) && !frameBuffer->push(frame))
            {
                
            }

            m_gameFrameIndex.fetch_add(1, std::memory_order_relaxed);

            assert(ZeroX::globalEngineLogger);
            ZeroX::globalEngineLogger->processMessages();
        }

        m_gameLibraryWatcher->stopWatching();
    }

    void EngineRuntime::stopGameThread()
    {
        m_runGame.store(false, std::memory_order_release);

        m_renderFrameIndex.store(0, std::memory_order_release);
        m_renderFrameIndex.notify_one();

        if (m_gameThread.joinable())
        {
            m_gameThread.join();
        }
    }

    void EngineRuntime::waitForWindowClose()
    {
        if (m_rendererThread.joinable())
        {
            m_rendererThread.join();
        }
    }

    void EngineRuntime::executeRenderLoop(renderer::FrameBuffer* frameBuffer)
    {
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

        if (glfwInit() != GLFW_TRUE)
        {
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

        GLFWwindow* window = glfwCreateWindow(1920, 1080, "Minimal Renderer", nullptr, nullptr);
        if (!window)
        {
            glfwTerminate();

            return;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            glfwDestroyWindow(window);
            glfwTerminate();

            return;
        }

        renderer::Allocator renderAllocator;
        renderer::allocateGraphicsResources(&renderAllocator, &graphicsConfig);
        renderer::MutableGraphicsMemory mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
        renderer::initializeGraphicsResources(mutableGraphicsMemory, &graphicsConfig);
        renderer::initializeGraphicsState();

        glfwSetWindowUserPointer(window, &mutableGraphicsMemory);

        int windowWidth{ 0 };
        int windowHeight{ 0 };
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        ::onWindowResize(window, windowWidth, windowHeight);
        glfwSetWindowSizeCallback(window, ::onWindowResize);
        glfwShowWindow(window);

        uint64_t renderFrameIndex       = 0;
        int64_t lastPresentationTime    = 0;
        auto startTimePoint             = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window))
        {
            if (!m_runRenderer.load(std::memory_order_acquire))
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                continue;
            }

            glfwPollEvents();

            renderer::Frame renderFrame;
            if (frameBuffer->pop(renderFrame))
            {
                renderer::processFrame(mutableGraphicsMemory, &renderFrame);
            }

            const glm::vec2 worldDimensions = renderer::getViewportWorldDimensions(
                mutableGraphicsMemory.camera.data(),
                mutableGraphicsMemory.cameraEye.data(),
                mutableGraphicsMemory.cameraFrustum.data());

            int clientWidth{ 0 };
            int clientHeight{ 0 };

            glfwGetWindowSize(window, &clientWidth, &clientHeight);

            renderer::clearFrameBuffer();
            renderer::render(mutableGraphicsMemory, renderer::getPixelsPerUnit(clientWidth, clientHeight, worldDimensions));

            glfwSwapBuffers(window);

            PFNGLXGETSYNCVALUESOMLPROC glXGetSyncValuesOML =
                (PFNGLXGETSYNCVALUESOMLPROC)glXGetProcAddressARB((const GLubyte *)"glXGetSyncValuesOML");

            int64_t systemTime          { 0 };
            int64_t streamCounter       { 0 };
            int64_t swapBufferCounter   { 0 };

            if (glXGetSyncValuesOML != nullptr && glXGetSyncValuesOML(nullptr, glfwGetGLXWindow(window), &systemTime, &streamCounter, &swapBufferCounter))
            {
                
            }
            else
            {
                auto timePoint = std::chrono::high_resolution_clock::now();

                systemTime = std::chrono::duration_cast<std::chrono::microseconds>(timePoint - startTimePoint).count();
            }

            m_frameDeltaTime.store(static_cast<double>(systemTime - lastPresentationTime) * 0.000001);

            m_renderFrameIndex.store(renderFrameIndex, std::memory_order_release);
            m_renderFrameIndex.notify_one();

            lastPresentationTime = systemTime;

            ++renderFrameIndex;
        }

        renderer::freeGraphicsResources(mutableGraphicsMemory);
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}