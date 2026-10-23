#pragma once

#include <future>
#include <memory>
#include <thread>

#include "config/zerox_game_config.h"
#include "game_library.h"
#include "game_library_watcher.h"
#include "zerox_audio_player.h"
#include "zerox_window.h"

namespace ZeroX
{
	struct AsyncLoadPromise
	{
		std::promise<IWindow*> window;
		std::promise<IAudioPlayer*> audioPlayer;
		std::promise<GameLibrary*> gameLibrary;
		std::promise<GameLibraryWatcher*> gameLibraryWatcher;
	};

	class EngineContext
	{
	public:
		EngineContext();
		~EngineContext();

		void load(const char* configPath, AsyncLoadPromise promise);

	private:
		void waitForLoadFinish();
		void loadAudioPlayer(const AudioConfig& audioConfig, std::promise<IAudioPlayer*> promise);
		void loadGameLibrary(const GameConfig& gameConfig, std::promise<GameLibrary*> libraryPromise, std::promise<GameLibraryWatcher*> watcherPromise);

		std::thread m_audioPlayerThread;
		std::thread m_gameLibraryThread;

		std::unique_ptr<IWindow> m_window							{ nullptr };
		std::unique_ptr<IAudioPlayer> m_audioPlayer					{ nullptr };
		std::unique_ptr<GameLibrary> m_gameLibrary					{ nullptr };
		std::unique_ptr<GameLibraryWatcher> m_gameLibraryWatcher	{ nullptr };
	};
}
