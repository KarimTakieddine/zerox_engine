#pragma once

#include <render_frame.h>
#include <zerox_entity_section.h>
#include <zerox_render_descriptor.h>

#include "zerox_memory_map.h"

namespace ZeroX
{
	using ZeroXBridge::EntitySection;
	using ZeroXBridge::EntitySectionFlags;
	using ZeroXBridge::EntitySectionType;
	using ZeroXBridge::RenderDescriptor;

	class FrameBuilder
	{
	public:
		void reset();
		void processRenderDescriptor(const std::byte* gameMemory, const MemoryMap& map, const RenderDescriptor& descriptor);

		const renderer::Frame& getFrame() const;

	private:
		void processEntitySection(const std::byte* gameMemory, const MemoryMap& map, EntitySection& section, uint64_t index);
		void allocateEntitySection(const MemoryMap& map, EntitySection& section);
		void processSpriteSection(const std::byte* gameMemory, EntitySection& section, uint64_t index);

		renderer::Frame m_frame;
	};
}
