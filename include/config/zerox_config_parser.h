#pragma once

#include <pugixml.hpp>

#include "zerox_game_config.h"
#include "zerox_window_config.h"

namespace ZeroX
{
	enum class ParseErrorCode : unsigned int
	{
		NONE				= 0,
		LOAD_ERROR			= 1,
		ENGINE_CONFIG_ERROR	= 2,
		WINDOW_CONFIG_ERROR	= 3,
		GAME_CONFIG_ERROR	= 4
		// etc...
	};

	struct ParseError
	{
		const char* description	{ "" };
		ParseErrorCode code		{ ParseErrorCode::NONE };
	};

	WindowConfig parseWindowConfig(const pugi::xml_node& node);
	GameConfig parseGameConfig(const pugi::xml_node& node);
	void setReasonableDefaults(WindowConfig& config);
	void setReasonableDefaults(GameConfig& config);
}
