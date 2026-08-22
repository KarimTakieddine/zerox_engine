#pragma once

#include "game_library_watcher.h"

namespace ZeroX
{
    class EngineRuntime
    {
    public:
        void startGameThread(const char* gameLibraryDir, const char* gameLibraryName);
        void startRenderThread();

        void stopGameThread();

    private:
        void executeGameLoop(const char* gameLibraryDir, const char* gameLibraryName);
        void executeRenderLoop();

        std::thread m_gameThread;
        std::thread m_rendererThread;
        std::atomic<bool> m_runGame{ true };
        std::atomic<bool> m_runRenderer{ true };
    };
}
