#include <render_entity.h>

#include "zerox_frame_builder.h"

namespace
{
	using renderer::Command;
	using renderer::CommandType;

	Command buildAllocateBatchCommand(size_t shaderProgramIndex, size_t vertexArrayIndex, size_t locationsIndex, size_t meshIndex, size_t entityCount)
	{
		Command command{ .type = CommandType::ALLOCATE_RENDER_BATCH };

		std::memcpy(command.data, &shaderProgramIndex, sizeof(size_t));
		std::memcpy(command.data + sizeof(size_t), &vertexArrayIndex, sizeof(size_t));
		std::memcpy(command.data + 2 * sizeof(size_t), &locationsIndex, sizeof(size_t));
		std::memcpy(command.data + 3 * sizeof(size_t), &meshIndex, sizeof(size_t));
		std::memcpy(command.data + 4 * sizeof(size_t), &entityCount, sizeof(size_t));

		return command;
	}

	using renderer::Transform;
	using renderer::Material;

	Command generateRenderCommand(const Transform& transform, size_t batchIndex, size_t entityIndex)
	{
		Command result{ .type = CommandType::UPDATE_ENTITY_TRANSFORM };

		std::memcpy(result.data, &batchIndex, sizeof(size_t));
		std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
		std::memcpy(result.data + 2 * sizeof(size_t), &transform, sizeof(Transform));

		return result;
	}

	Command generateRenderCommand(const Material& material, size_t batchIndex, size_t entityIndex)
	{
		Command result{ .type = renderer::CommandType::UPDATE_ENTITY_MATERIAL };

		std::memcpy(result.data, &batchIndex, sizeof(size_t));
		std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
		std::memcpy(result.data + 2 * sizeof(size_t), &material, sizeof(Material));

		return result;
	}
}

namespace ZeroX
{
	void FrameBuilder::reset()
	{
		m_frame.renderCommands.clear();
	}

	void FrameBuilder::processRenderDescriptor(const std::byte* gameMemory, const MemoryMap& map, const RenderDescriptor& descriptor)
	{
		reset();

		for (uint64_t i = 0; i < descriptor.entitySectionCount; ++i)
		{
			processEntitySection(gameMemory, map, *(descriptor.entitySections + i), i);
		}
	}

	void FrameBuilder::processEntitySection(const std::byte* gameMemory, const MemoryMap& map, EntitySection& section, uint64_t index)
	{
		if ( ( section.flags & EntitySectionFlags::ALLOCATE_BATCH ) == EntitySectionFlags::ALLOCATE_BATCH )
		{
			allocateEntitySection(map, section);
		}

		switch (section.type)
		{
		case EntitySectionType::SPRITE:
			processSpriteSection(gameMemory, section, index);
			break;
		default:
			break;
		}
	}

	const renderer::Frame& FrameBuilder::getFrame() const
	{
		return m_frame;
	}

	void FrameBuilder::allocateEntitySection(const MemoryMap& map, EntitySection& section)
	{
		switch (section.type)
		{
		case EntitySectionType::SPRITE:
		{
			m_frame.renderCommands.push_back(
				::buildAllocateBatchCommand(
					map.getShaderProgramIndex(ShaderProgramType::SPRITE),
					map.getVertexArrayIndex(VertexArrayType::VERTEX_COLORED_QUAD),
					map.getLocationsDescriptorIndex(ShaderProgramType::SPRITE),
					map.getMeshIndex(MeshType::QUAD),
					section.descriptorCount
				));

			section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;

			break;
		}
		default:
			break;
		}
	}

	void FrameBuilder::processSpriteSection(const std::byte* gameMemory, EntitySection& section, uint64_t index)
	{
		for (uint64_t i = 0; i < section.descriptorCount; ++i)
		{
			const auto* readDescriptor = section.readDescriptors + i;

			const renderer::RenderEntity* renderEntity = reinterpret_cast<const renderer::RenderEntity*>(gameMemory + readDescriptor->start + readDescriptor->offset);

			m_frame.renderCommands.push_back(::generateRenderCommand(renderEntity->transform, index, i));
			m_frame.renderCommands.push_back(::generateRenderCommand(renderEntity->material, index, i));
		}
	}
}