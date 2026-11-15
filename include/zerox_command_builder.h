#pragma once

#include <zerox_entity_section.h>
#include <zerox_render_descriptor.h>

#include "zerox_command_buffer.h"
#include "zerox_memory_map.h"

namespace ZeroX
{
	using ZeroXBridge::EntitySection;
	using ZeroXBridge::RenderDescriptor;

	class CommandBufferBuilder
	{
	public:
		void reset();
		void processRenderDescriptor(const std::byte* gameMemory, const MemoryMap& map, const RenderDescriptor& descriptor);

		const CommandBuffer& getBuffer() const;

		template<CommandType C, typename Params>
		void addCommand(const Params& params)
		{
			RenderCommand renderCommand{ .type = C };
			renderCommand.paramData.resize(sizeof(Params));
			std::memcpy(renderCommand.paramData.data(), &params, sizeof(Params));
			m_buffer.commands.push_back(std::move(renderCommand));
		}

	private:
		void processEntitySection(const std::byte* gameMemory, const MemoryMap& map, EntitySection& section, uint64_t index);
		void allocateEntitySection(const MemoryMap& map, EntitySection& section);
		void processBackgroundSection(const std::byte* gameMemory, EntitySection& section, uint64_t index);
		void processSpriteSection(const std::byte* gameMemory, EntitySection& section, uint64_t index);

		CommandBuffer m_buffer;
	};
}
