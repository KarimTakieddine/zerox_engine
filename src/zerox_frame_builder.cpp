#include <render_batch.h>
#include <render_entity.h>
#include <cstring>

#include <memory_cursor.hpp>

#include "zerox_frame_builder.h"

namespace
{
	using renderer::Command;
	using renderer::CommandType;
	using renderer::DrawMode;

	Command buildAllocateBatchCommand(size_t shaderProgramIndex, size_t vertexArrayIndex, size_t locationsIndex, size_t meshIndex, size_t entityCount, DrawMode drawMode)
	{
		Command command{ .type = CommandType::ALLOCATE_RENDER_BATCH };

		std::memcpy(command.data, &shaderProgramIndex, sizeof(size_t));
		std::memcpy(command.data + sizeof(size_t), &vertexArrayIndex, sizeof(size_t));
		std::memcpy(command.data + 2 * sizeof(size_t), &locationsIndex, sizeof(uint64_t));
		std::memcpy(command.data + 2 * sizeof(size_t) + sizeof(uint64_t), &meshIndex, sizeof(size_t));
		std::memcpy(command.data + 3 * sizeof(size_t) + sizeof(uint64_t), &entityCount, sizeof(size_t));
		std::memcpy(command.data + 4 * sizeof(size_t) + sizeof(uint64_t), &drawMode, sizeof(DrawMode));

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

	Command generateSetLayerCommand(uint64_t layer, size_t batchIndex, size_t entityIndex)
	{
		Command result{ .type = renderer::CommandType::SET_TEXTURE_LAYER };

		std::memcpy(result.data, &batchIndex, sizeof(size_t));
		std::memcpy(result.data + sizeof(size_t), &entityIndex, sizeof(size_t));
		std::memcpy(result.data + 2 * sizeof(size_t), &layer, sizeof(uint64_t));

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
			processEntitySection(gameMemory, map, *( descriptor.entitySections + i ), i);
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
		case EntitySectionType::BACKGROUND:
			processBackgroundSection(gameMemory, section, index);
			break;
		case EntitySectionType::SPRITE:
		case EntitySectionType::COLLIDER:
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
		case EntitySectionType::BACKGROUND:
		{
			m_frame.renderCommands.push_back(
				::buildAllocateBatchCommand(
					map.getShaderProgramIndex(ShaderProgramType::BACKGROUND),
					map.getVertexArrayIndex(VertexArrayType::TEXTURED_QUAD),
					map.getLocationsDescriptorIndex(ShaderProgramType::BACKGROUND),
					map.getMeshIndex(MeshType::QUAD),
					section.descriptorCount,
					DrawMode::TRIANGLES
				));

			section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;
			
			break;
		}
		case EntitySectionType::SPRITE:
		{
			m_frame.renderCommands.push_back(
				::buildAllocateBatchCommand(
					map.getShaderProgramIndex(ShaderProgramType::SPRITE),
					map.getVertexArrayIndex(VertexArrayType::TEXTURED_QUAD),
					map.getLocationsDescriptorIndex(ShaderProgramType::SPRITE),
					map.getMeshIndex(MeshType::QUAD),
					section.descriptorCount,
					DrawMode::TRIANGLES
				));

			section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;

			break;
		}
		case EntitySectionType::COLLIDER:
		{
			m_frame.renderCommands.push_back(
				::buildAllocateBatchCommand(
					map.getShaderProgramIndex(ShaderProgramType::COLLIDER),
					map.getVertexArrayIndex(VertexArrayType::COLLIDER),
					map.getLocationsDescriptorIndex(ShaderProgramType::COLLIDER),
					map.getMeshIndex(MeshType::COLLIDER),
					section.descriptorCount,
					DrawMode::LINES
				));

			section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;
		}
		default:
			break;
		}
	}

	void FrameBuilder::processBackgroundSection(const std::byte* gameMemory, EntitySection& section, uint64_t index)
	{
		for (uint64_t i = 0; i < section.descriptorCount; ++i)
		{
			const EntityReadDescriptor* readDescriptor = section.readDescriptors + i;

			MemoryCursor<16> cursor(readDescriptor->start + readDescriptor->offset);

			const auto* renderTransform = reinterpret_cast<const renderer::Transform*>(gameMemory + cursor.getOffset());
			cursor.step<renderer::Transform>();

			const auto* renderMaterial = reinterpret_cast<const renderer::Material*>(gameMemory + cursor.getOffset());
			cursor.step<renderer::Material>();

			const auto* layer = reinterpret_cast<const uint64_t*>(gameMemory + cursor.getOffset() + sizeof(glm::vec2));

			m_frame.renderCommands.push_back(::generateRenderCommand(*renderTransform, index, i));
			m_frame.renderCommands.push_back(::generateRenderCommand(*renderMaterial, index, i));
			m_frame.renderCommands.push_back(::generateSetLayerCommand(*layer, index, i));
		}
	}

	void FrameBuilder::processSpriteSection(const std::byte* gameMemory, EntitySection& section, uint64_t index)
	{
		for (uint64_t i = 0; i < section.descriptorCount; ++i)
		{
			const EntityReadDescriptor* readDescriptor = section.readDescriptors + i;

			MemoryCursor<16> cursor(readDescriptor->start + readDescriptor->offset);

			const auto* renderTransform = reinterpret_cast<const renderer::Transform*>(gameMemory + cursor.getOffset());
			cursor.step<renderer::Transform>();

			const auto* renderMaterial = reinterpret_cast<const renderer::Material*>(gameMemory + cursor.getOffset());
			cursor.step<renderer::Material>();

			m_frame.renderCommands.push_back(::generateRenderCommand(*renderTransform, index, i));
			m_frame.renderCommands.push_back(::generateRenderCommand(*renderMaterial, index, i));
		}
	}
}