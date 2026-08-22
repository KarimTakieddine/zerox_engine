#include <filesystem>

#include "zerox_engine_runtime.h"

#include <iostream>

namespace ZeroX
{
    void EngineRuntime::startGameThread(const char* gameLibraryDir, const char* gameLibraryName)
    {
        m_gameThread = std::thread(&EngineRuntime::executeGameLoop, this, gameLibraryDir, gameLibraryName);
    }

    void EngineRuntime::startRenderThread()
    {

    }

    void EngineRuntime::executeGameLoop(const char* gameLibraryDir, const char* gameLibraryName)
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

        while (m_runGame.load(std::memory_order_acquire))
        {
            const uint64_t watchCounter = gameLibraryWatcher.getChangeCounter();

            if (fileChangeCounter != watchCounter)
            {
                std::cout << "File has changed" << std::endl;

                const auto filenamePath = gameLibraryPath.filename();

                std::string filename        = filenamePath.string();
                const std::string extension = filenamePath.extension().string();

                const auto pos = filename.find(extension);
                if (pos == std::string::npos)
                {
                    std::cout << "Failed to find extension in game library file" << std::endl;
                }

                filename.insert(pos, "_" + std::to_string(watchCounter));

                const auto destinationPath = gameDirectoryPath / filename;

                if (!platform::copyFile(gameLibraryPath.c_str(), destinationPath.c_str()))
                {
                    std::cout << "Failed to copy game library file from: " << gameLibraryPath.string() << " to " << destinationPath.string() << std::endl;

                    continue;
                }
                
                std::cout << "Copy created: " << destinationPath.string() << std::endl;

                

                fileChangeCounter = watchCounter;
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