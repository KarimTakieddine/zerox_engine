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
		LAST					= SET_CLIENT_AREA
	};

	struct RenderCommand
	{
		std::vector<std::byte> paramData;
		CommandType type{ CommandType::UNKNOWN };
	};

	struct CommandBuffer
	{
		std::vector<RenderCommand> commands;
	};
}
