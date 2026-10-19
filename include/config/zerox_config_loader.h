#pragma once

#include <thread>
#include <future>

#include "zerox_game_config.h"
#include "zerox_window_config.h"

namespace ZeroX
{
	class ConfigLoader
	{
	public:
		~ConfigLoader();
		std::future<GameConfig> getGameConfigFuture();
		std::future<WindowConfig> getWindowConfigFuture();
		void load(const char* file);

	private:
		void execute(const char* file);

		std::thread m_loadThread;
		std::promise<GameConfig> m_gameConfigPromise;
		std::promise<WindowConfig> m_windowConfigPromise;
	};
}
