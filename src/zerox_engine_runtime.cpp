#include <filesystem>
#include <iostream>

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

#include "game_library.h"
#include "render_command_generator.h"
#include "zerox_engine_runtime.h"

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
            std::cout << "Failed to find extension in game library file" << std::endl;
        }

        filename.insert(pos, "_" + std::to_string(suffix));

        destinationPath = gameDirectoryPath / filename;

        return platform::copyFile(gameLibraryPath.c_str(), destinationPath.c_str());
    }

    bool reloadGameLibrary(const std::filesystem::path gameDirectoryPath, const std::filesystem::path& gameLibraryPath, std::atomic<std::shared_ptr<ZeroX::GameLibrary>>& gameLibrary, uint64_t suffix)
    {
        if (platform::getFileSize(gameLibraryPath.c_str()) == 0)
        {
            return false;
        }

        std::filesystem::path destinationPath;
        if (!::copyGameLibrary(gameDirectoryPath, gameLibraryPath, destinationPath, suffix))
        {
            std::cout << "Failed to copy file from: " << gameLibraryPath.c_str() << " to " << destinationPath.c_str() << std::endl;
            return false;
        }

        if (!gameLibrary.load(std::memory_order_acquire)->load(destinationPath.c_str()))
        {
            return false;
        }

        return true;
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
    void EngineRuntime::startGameThread(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer)
    {
        m_gameThread = std::thread(&EngineRuntime::executeGameLoop, this, gameLibraryDir, gameLibraryName, frameBuffer);
    }

    void EngineRuntime::startRenderThread(renderer::FrameBuffer* frameBuffer)
    {
        m_rendererThread = std::thread(&EngineRuntime::executeRenderLoop, this, frameBuffer);
    }

    void EngineRuntime::executeGameLoop(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer)
    {
        ZeroX::GameLibraryWatcher gameLibraryWatcher;

        uint64_t fileChangeCounter = 0;

        const std::filesystem::path gameDirectoryPath(gameLibraryDir);
        if (!std::filesystem::is_directory(gameDirectoryPath))
        {
            return;
        }

        if (!gameLibraryWatcher.startWatching(gameLibraryDir, gameLibraryName))
        {
            return;
        }

        const std::filesystem::path gameLibraryPath = gameDirectoryPath / gameLibraryName;
        if (!std::filesystem::exists(gameLibraryPath))
        {
            return;
        }

        BumpAllocator<16> gameAllocator;
        gameAllocator.allocate(4096);

        auto gameLibrary = std::make_shared<GameLibrary>();
        if (!gameLibrary->load(gameLibraryPath.c_str()))
        {
            std::cout << "Failed to load game library" << std::endl;
            return;
        }

        gameLibrary->getFunctions().initGame(&gameAllocator);

        std::atomic<std::shared_ptr<GameLibrary>> sharedGameLibrary(gameLibrary);

        m_gameFrameIndex.store(0, std::memory_order_relaxed);

        while (m_runGame.load(std::memory_order_acquire))
        {
            const uint64_t watchCounter = gameLibraryWatcher.getChangeCounter();

            if (fileChangeCounter != watchCounter)
            {
                if (!::reloadGameLibrary(gameDirectoryPath, gameLibraryPath, sharedGameLibrary, watchCounter))
                {
                    std::cout << "Failed to load game library" << std::endl;
                }
                else
                {
                    std::cout << "Re-loaded game library" << std::endl;
                }

                fileChangeCounter = gameLibraryWatcher.getChangeCounter();
            }

            auto sharedLibrary = sharedGameLibrary.load(std::memory_order_acquire);
            if (!sharedLibrary || !sharedLibrary->isLoaded() && !sharedLibrary->isValid())
            {
                std::cout << "No shared game library found or successfully loaded" << std::endl;
                continue;
            }

            // Wait for frame N - maxQueuedFrames

            if (m_gameFrameIndex.load(std::memory_order_relaxed) >= 4)
            {
                const uint64_t renderFrameIndex = m_renderFrameIndex.load(std::memory_order_acquire);
                const uint64_t gameFrameIndex   = m_gameFrameIndex.load(std::memory_order_relaxed);

                uint64_t difference = gameFrameIndex - renderFrameIndex;

                if (difference >= 4)
                {
                    m_renderFrameIndex.wait(renderFrameIndex);
                }
            }

            sharedLibrary->getFunctions().updateGame(&gameAllocator, m_frameDeltaTime.load(std::memory_order_acquire));

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
        }

        gameLibraryWatcher.stopWatching();
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
            .fov    = 45.0f,
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

            renderer::clearFrameBuffer();
            renderer::render(renderer::freezeGraphicsMemory(mutableGraphicsMemory));

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