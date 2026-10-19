#pragma once

#include <memory>

#include "game_library.h"
#include "game_library_watcher.h"
#include "zerox_audio_player.h"
#include "zerox_window.h"

namespace ZeroX
{
	struct EngineContext
	{
		std::unique_ptr<IAudioPlayer> audioPlayer				{ nullptr };
		std::unique_ptr<GameLibrary> gameLibrary				{ nullptr };
		std::unique_ptr<GameLibraryWatcher> gameLibraryWatcher	{ nullptr };
	};
}
