#pragma once

#include <render_frame_buffer.h>

#include "game_library_watcher.h"

namespace ZeroX
{
    class EngineRuntime
    {
    public:
        void startGameThread(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer);
        void startRenderThread();

        void stopGameThread();

    private:
        void executeGameLoop(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer);
        void executeRenderLoop();

        std::thread m_gameThread;
        std::thread m_rendererThread;
        std::atomic<bool> m_runGame{ true };
        std::atomic<bool> m_runRenderer{ true };
    };
}
