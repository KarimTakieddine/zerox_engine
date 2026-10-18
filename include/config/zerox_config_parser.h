#pragma once

#include "zerox_engine_config.h"

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

	bool parseEngineConfig(const char* file, EngineConfig& config, ParseError* error);
	void setReasonableDefaults(EngineConfig& config);
}
