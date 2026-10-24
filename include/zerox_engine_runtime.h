#pragma once

#include <render_frame_buffer.h>

#include "zerox_engine_context.h"

namespace ZeroX
{
    class EngineRuntime
    {
    public:
        EngineRuntime();
        ~EngineRuntime();

        void load(const char* configPath);

    private:
        void render(IWindow* window, renderer::FrameBuffer* frameBuffer);
        void stopRendering();

        std::unique_ptr<EngineContext> m_context{ nullptr };

        std::thread m_rendererThread;
        std::atomic<uint64_t> m_renderFrameIndex{ 0 };
        std::atomic<double> m_frameDeltaTime    { 0.0 };
        std::atomic<bool> m_runRenderer         { true };
    };
}
