#pragma once

#include <cstdint>
#include <vector>

namespace ZeroX
{
	enum class CommandType : uint8_t
	{
		UNKNOWN					= 0,
		ALLOCATE_RENDER_BATCH	= 1,
		UPDATE_ENTITY_TRANSFORM	= 2,
		UPDATE_ENTITY_MATERIAL	= 3,
		SET_TEXTURE_LAYER		= 4,
		SET_CLIENT_AREA			= 5,
		ALLOCATE_AUDIO			= 6,
		PLAY_AUDIO				= 7,
		SHUTDOWN_AUDIO			= 8,
		UPLOAD_TEXTURES			= 9,
		UPLOAD_TEXTURE_ARRAY	= 10,
		LAST					= UPLOAD_TEXTURE_ARRAY
	};

	struct EngineCommand
	{
		std::vector<std::byte> paramData;
		CommandType type{ CommandType::UNKNOWN };
	};

	struct CommandBuffer
	{
		std::vector<EngineCommand> commands;
	};
}
