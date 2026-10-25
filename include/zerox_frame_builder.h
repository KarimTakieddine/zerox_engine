#pragma once

#include <render_frame.h>
#include <zerox_entity_section.h>

namespace ZeroX
{
	using ZeroXBridge::EntitySection;

	class FrameBuilder
	{
	public:
		const renderer::Frame& getFrame() const;
		void renderEntitySection(const EntitySection& section);

	private:
		renderer::Frame m_frame;
	};
}
