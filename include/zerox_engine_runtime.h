#pragma once

#include <render_frame_buffer.h>

#include "game_library_watcher.h"

namespace ZeroX
{
    class EngineRuntime
    {
    public:
        void startGameThread(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer);
        void startRenderThread(renderer::FrameBuffer* frameBuffer);

        void stopGameThread();
        void waitForWindowClose();

    private:
        void executeGameLoop(const char* gameLibraryDir, const char* gameLibraryName, renderer::FrameBuffer* frameBuffer);
        void executeRenderLoop(renderer::FrameBuffer* frameBuffer);

        std::thread m_gameThread;
        std::thread m_rendererThread;
        std::atomic<uint64_t> m_gameFrameIndex{ 0 };
        std::atomic<uint64_t> m_renderFrameIndex{ 0 };
        std::atomic<bool> m_runGame{ true };
        std::atomic<bool> m_runRenderer{ true };
    };
}
