#pragma once

#include <future>

#include <pugixml.hpp>

#include "zerox_audio_config.h"
#include "zerox_game_config.h"
#include "zerox_graphics_config.h"
#include "zerox_window_config.h"

namespace ZeroX
{
	std::future<WindowConfig> loadWindowConfig(const pugi::xml_node& node);
	std::future<AudioConfig> loadAudioConfig(const pugi::xml_node& node);
	std::future<GameConfig> loadGameConfig(const pugi::xml_node& node);

	std::future<GraphicsConfig> loadGraphicsConfig();
}
