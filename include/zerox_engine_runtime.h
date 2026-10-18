#pragma once

#include <render_frame_buffer.h>

#include "game_library.h"
#include "game_library_watcher.h"
#include "zerox_audio_player.h"
#include "config/zerox_game_config.h"
#include "config/zerox_window_config.h"
#include "zerox_window.h"

namespace ZeroX
{
    class EngineRuntime
    {
    public:
        EngineRuntime(const WindowConfig& config);
        ~EngineRuntime();

        void run(const GameConfig& gameConfig, const AudioConfig& audioConfig);
        void stopRendering();

    private:
        void render(renderer::FrameBuffer* frameBuffer);
        bool initializeGameLibrary(const GameConfig& config);
        bool startWatchingGameLibrary(const GameConfig& config);
        bool reloadGameLibrary(const GameConfig& config, uint64_t changeCounter);

        std::unique_ptr<IWindow> m_window                       { nullptr };
        std::unique_ptr<IAudioPlayer> m_audioPlayer             { nullptr };
        std::unique_ptr<GameLibraryWatcher> m_gameLibraryWatcher{ nullptr };
        std::unique_ptr<GameLibrary> m_gameLibrary              { nullptr };

        std::thread m_rendererThread;
        std::atomic<uint64_t> m_renderFrameIndex{ 0 };
        std::atomic<double> m_frameDeltaTime    { 0.0 };
        std::atomic<bool> m_runRenderer         { true };
    };
}
