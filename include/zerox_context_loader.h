#pragma once

#include <future>
#include <memory>
#include <thread>

#include "config/zerox_config_loader.h"
#include "zerox_engine_context.h"

namespace ZeroX
{
	class ContextLoader
	{
	public:
		~ContextLoader();
		ContextLoader();

		std::future<WindowConfig> getWindowConfigFuture();
		std::future<IAudioPlayer*> getAudioPlayerFuture();
		std::future<GameLibrary*> getGameLibraryFuture();
		std::future<GameLibraryWatcher*> getGameLibraryWatcherFuture();

		void load(const char* configFile);
		EngineContext* getEngineContext() const;

	private:
		void loadAudio(const AudioConfig& config);
		void loadGame(const GameConfig& config);

		std::thread m_audioLoadThread;
		std::thread m_gameLoadThread;

		std::promise<IAudioPlayer*> m_audioPlayerPromise;
		std::promise<GameLibrary*> m_gameLibraryPromise;
		std::promise<GameLibraryWatcher*> m_gameLibraryWatcherPromise;

		std::unique_ptr<ConfigLoader> m_configLoader	{ nullptr };
		std::unique_ptr<EngineContext> m_engineContext	{ nullptr };
	};
}
