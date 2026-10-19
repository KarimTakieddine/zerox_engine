#pragma once

#include <thread>
#include <future>

#include "zerox_audio_config.h"
#include "zerox_game_config.h"
#include "zerox_window_config.h"

namespace ZeroX
{
	class ConfigLoader
	{
	public:
		~ConfigLoader();

		std::future<WindowConfig> getWindowConfigFuture();
		std::future<AudioConfig> getAudioConfigFuture();
		std::future<GameConfig> getGameConfigFuture();

		void load(const char* file);

	private:
		void execute(const char* file);

		std::thread m_loadThread;
		std::promise<WindowConfig> m_windowConfigPromise;
		std::promise<AudioConfig> m_audioConfigPromise;
		std::promise<GameConfig> m_gameConfigPromise;
	};
}
