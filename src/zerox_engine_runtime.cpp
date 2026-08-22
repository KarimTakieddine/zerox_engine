#include <filesystem>
#include <iostream>

#include <zerox_game_memory.hpp>

#include "game_library.h"
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
}

namespace ZeroX
{
    void EngineRuntime::startGameThread(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer)
    {
        m_gameThread = std::thread(&EngineRuntime::executeGameLoop, this, gameLibraryDir, gameLibraryName, frameBuffer);
    }

    void EngineRuntime::startRenderThread()
    {

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
            }

            sharedLibrary->getFunctions().updateGame(&gameAllocator);

            ZeroXGame::GameMemory gameMemory = ZeroXGame::readGameMemory(&gameAllocator);

            renderer::Frame frame;

            const auto players = gameMemory.players;
            for (size_t i = 0; i < players.size(); ++i)
            {
                ZeroXGame::GameEntity* playerEntity = players.data() + i;

                renderer::Command updateTransformCommand{ .type = renderer::CommandType::UPDATE_ENTITY_TRANSFORM };

                size_t batchIndex   = 0;
                size_t entityIndex  = i;

                const float speed = i * 0.25f;

                playerEntity->transform.localToWorld[3].x += 0.016f * speed;

                std::memcpy(updateTransformCommand.data, &batchIndex, sizeof(size_t));
                std::memcpy(updateTransformCommand.data + sizeof(size_t), &entityIndex, sizeof(size_t));
                std::memcpy(updateTransformCommand.data + 2 * sizeof(size_t), &(playerEntity->transform), sizeof(ZeroXGame::Transform));

                frame.renderCommands.push_back({ updateTransformCommand });
            }

            while(m_runGame.load(std::memory_order_acquire) && !frameBuffer->push(frame))
            {
                
            }
        }

        gameLibraryWatcher.stopWatching();
    }

    void EngineRuntime::stopGameThread()
    {
        m_runGame.store(false);

        if (m_gameThread.joinable())
        {
            m_gameThread.join();
        }
    }

    void EngineRuntime::executeRenderLoop()
    {

    }
}