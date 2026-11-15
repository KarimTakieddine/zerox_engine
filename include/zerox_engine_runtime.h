#pragma once

#include "config/zerox_graphics_config.h"
#include "zerox_engine_context.h"

namespace ZeroX
{
    struct CommandPool;

    class EngineRuntime
    {
    public:
        EngineRuntime();
        ~EngineRuntime();

        void load(const char* configPath);

    private:
        void render(const GraphicsConfig& config, IWindow* window, CommandPool* commandPool);
        void stopRendering();

        std::unique_ptr<EngineContext> m_context{ nullptr };

        std::thread m_rendererThread;
        std::atomic<uint64_t> m_renderFrameIndex{ 0 };
        std::atomic<double> m_frameDeltaTime    { 0.0 };
        std::atomic<bool> m_runRenderer         { true };

#ifdef _WIN32
        std::atomic<UINT> m_resizeWidth     { 0 };
        std::atomic<UINT> m_resizeHeight    { 0 };
        std::atomic<bool> m_resizeRequested { false };
#endif
    };
}
