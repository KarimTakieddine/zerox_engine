#include <cstring>

#include <memory_cursor.hpp>

#include <render_batch.h>
#include <render_command_types.h>

#include "zerox_frame_builder.h"

namespace
{
	template<renderer::CommandType R, typename T>
	renderer::Command buildRenderCommand(const T& command)
	{
		renderer::Command result{ .type = R };
		std::memcpy(result.data, &command, sizeof(T));
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
				::buildRenderCommand<renderer::CommandType::ALLOCATE_RENDER_BATCH, renderer::AllocateBatchCommand>({
					.shaderProgramIndex = map.getShaderProgramIndex(ShaderProgramType::BACKGROUND),
					.vertexArrayIndex	= map.getVertexArrayIndex(VertexArrayType::TEXTURED_QUAD),
					.locationsIndex		= map.getLocationsDescriptorIndex(ShaderProgramType::BACKGROUND),
					.meshIndex			= map.getMeshIndex(MeshType::QUAD),
					.entityCount		= section.descriptorCount,
					.drawMode			= static_cast<uint64_t>(renderer::DrawMode::TRIANGLES)
				}));

			section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;
			
			break;
		}
		case EntitySectionType::SPRITE:
		{
			m_frame.renderCommands.push_back(
				::buildRenderCommand<renderer::CommandType::ALLOCATE_RENDER_BATCH, renderer::AllocateBatchCommand>({
					.shaderProgramIndex = map.getShaderProgramIndex(ShaderProgramType::SPRITE),
					.vertexArrayIndex	= map.getVertexArrayIndex(VertexArrayType::TEXTURED_QUAD),
					.locationsIndex		= map.getLocationsDescriptorIndex(ShaderProgramType::SPRITE),
					.meshIndex			= map.getMeshIndex(MeshType::QUAD),
					.entityCount		= section.descriptorCount,
					.drawMode			= static_cast<uint64_t>(renderer::DrawMode::TRIANGLES)
				}));

			section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;

			break;
		}
		case EntitySectionType::COLLIDER:
		{
			m_frame.renderCommands.push_back(
				::buildRenderCommand<renderer::CommandType::ALLOCATE_RENDER_BATCH, renderer::AllocateBatchCommand>({
					.shaderProgramIndex = map.getShaderProgramIndex(ShaderProgramType::COLLIDER),
					.vertexArrayIndex	= map.getVertexArrayIndex(VertexArrayType::COLLIDER),
					.locationsIndex		= map.getLocationsDescriptorIndex(ShaderProgramType::COLLIDER),
					.meshIndex			= map.getMeshIndex(MeshType::COLLIDER),
					.entityCount		= section.descriptorCount,
					.drawMode			= static_cast<uint64_t>(renderer::DrawMode::LINES)
				}));

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

			m_frame.renderCommands.push_back(::buildRenderCommand<renderer::CommandType::UPDATE_ENTITY_TRANSFORM, renderer::UpdateTransformCommand>({ *renderTransform, index, i }));
			m_frame.renderCommands.push_back(::buildRenderCommand<renderer::CommandType::UPDATE_ENTITY_MATERIAL, renderer::UpdateMaterialCommand>({ *renderMaterial, index, i }));

			renderer::SetTextureLayerCommand command;
			command.batchIndex	= index;
			command.entityIndex = i;
			command.layer		= *layer;
			m_frame.renderCommands.push_back(::buildRenderCommand<renderer::CommandType::SET_TEXTURE_LAYER, renderer::SetTextureLayerCommand>(command));
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

			m_frame.renderCommands.push_back(::buildRenderCommand<renderer::CommandType::UPDATE_ENTITY_TRANSFORM, renderer::UpdateTransformCommand>({ *renderTransform, index, i }));
			m_frame.renderCommands.push_back(::buildRenderCommand<renderer::CommandType::UPDATE_ENTITY_MATERIAL, renderer::UpdateMaterialCommand>({ *renderMaterial, index, i }));
		}
	}
}