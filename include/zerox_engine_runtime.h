#pragma once

#include <semaphore>

#include "config/zerox_graphics_config.h"
#include "zerox_command_pool.h"
#include "zerox_engine_context.h"

namespace ZeroX
{
    struct CommandPool;

    inline constexpr size_t MAX_AUDIO_COMMANDS = 1 << 4;

    class EngineRuntime
    {
    public:
        EngineRuntime();
        ~EngineRuntime();

        void load(const char* configPath);

    private:
        void render(const GraphicsConfig& config, IWindow* window, CommandPool* commandPool);
        void processAudio(IAudioPlayer* player, CommandPool* commandPool);
        void stopRendering();
        void stopPlayingAudio();

        void allocateAudioFile(const AudioFile& file);
        void playAudio(size_t index);
        void uploadTextures(const char* const* paths, const ZeroXBridge::Texture* textures, size_t count);
        void uploadTextureArray(const char* const* paths, size_t count, const ZeroXBridge::Texture* texture, size_t index);

        std::unique_ptr<EngineContext> m_context            { nullptr };
        std::unique_ptr<CommandPool> m_renderCommandPool    { nullptr };
        std::unique_ptr<CommandPool> m_audioCommandPool     { nullptr };

        std::thread m_rendererThread;
        std::thread m_audioThread;
        std::counting_semaphore<MAX_AUDIO_COMMANDS> m_audioCommandSemaphore { 0 };
        std::atomic<uint64_t> m_renderFrameIndex                            { 0 };
        std::atomic<double> m_frameDeltaTime                                { 0.0 };
        std::atomic<bool> m_runRenderer                                     { true };
        std::atomic<bool> m_runAudio                                        { true };

#ifdef _WIN32
        std::atomic<UINT> m_resizeWidth     { 0 };
        std::atomic<UINT> m_resizeHeight    { 0 };
        std::atomic<bool> m_resizeRequested { false };
#endif
    };
}
