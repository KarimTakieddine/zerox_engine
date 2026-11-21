#include <cstring>

#include <zerox_entity_section.h>
#include <zerox_render_entity.h>

#include <renderer.hpp>

#include "zerox_command_builder.h"

namespace ZeroX
{
	using ZeroXBridge::EntitySectionFlags;
	using ZeroXBridge::EntitySectionType;
	using ZeroXBridge::EntityReadDescriptor;

	void CommandBufferBuilder::reset()
	{
		m_buffer.commands.clear();
	}

	void CommandBufferBuilder::processRenderDescriptor(const std::byte* gameMemory, const MemoryMap& map, const RenderDescriptor& descriptor)
	{
		reset();

		for (uint64_t i = 0; i < descriptor.entitySectionCount; ++i)
		{
			processEntitySection(gameMemory, map, *( descriptor.entitySections + i ), i);
		}
	}

	void CommandBufferBuilder::processEntitySection(const std::byte* gameMemory, const MemoryMap& map, EntitySection& section, uint64_t index)
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
		case EntitySectionType::TILED_SPRITE:
		case EntitySectionType::COLLIDER:
			processSpriteSection(gameMemory, section, index);
			break;
		default:
			break;
		}
	}

	const CommandBuffer& CommandBufferBuilder::getBuffer() const
	{
		return m_buffer;
	}

	void CommandBufferBuilder::allocateEntitySection(const MemoryMap& map, EntitySection& section)
	{
		ShaderProgramType shaderProgramType	{ ShaderProgramType::UNKNOWN };
		VertexArrayType vertexArrayType		{ VertexArrayType::UNKNOWN };
		MeshType meshType					{ MeshType::UNKNOWN };
		renderer::DrawMode drawMode			{ renderer::DrawMode::TRIANGLES };

		switch (section.type)
		{
		case EntitySectionType::BACKGROUND:
		{
			shaderProgramType	= ShaderProgramType::BACKGROUND;
			vertexArrayType		= VertexArrayType::TEXTURED_QUAD;
			meshType			= MeshType::QUAD;

			break;
		}
		case EntitySectionType::SPRITE:
		{
			shaderProgramType	= ShaderProgramType::SPRITE;
			vertexArrayType		= VertexArrayType::TEXTURED_QUAD;
			meshType			= MeshType::QUAD;

			break;
		}
		case EntitySectionType::TILED_SPRITE:
		{
			shaderProgramType	= ShaderProgramType::TILED_SPRITE;
			vertexArrayType		= VertexArrayType::TEXTURED_QUAD;
			meshType			= MeshType::QUAD;

			break;
		}
		case EntitySectionType::COLLIDER:
		{
			shaderProgramType	= ShaderProgramType::COLLIDER;
			vertexArrayType		= VertexArrayType::COLLIDER;
			meshType			= MeshType::COLLIDER;
			drawMode			= renderer::DrawMode::LINES;

			break;
		}
		default:
			return ;
		}

		renderer::AllocateBatchCommand allocateBatchCommand = {
			.entityCount		= static_cast<size_t>(section.descriptorCount),
			.drawMode			= drawMode,
			.meshIndex			= map.getMeshIndex(meshType),
			.shaderProgramIndex	= map.getShaderProgramIndex(shaderProgramType),
			.vertexArrayIndex	= map.getVertexArrayIndex(vertexArrayType),
			.elementCount		= 6 /* TODO(Karim) */
		};

		RenderCommand command{ .type = CommandType::ALLOCATE_RENDER_BATCH };
		command.paramData.resize(sizeof(renderer::AllocateBatchCommand));
		std::memcpy(command.paramData.data(), &allocateBatchCommand, sizeof(renderer::AllocateBatchCommand));
		m_buffer.commands.push_back(std::move(command));

		section.flags &= ~EntitySectionFlags::ALLOCATE_BATCH;
	}

	void CommandBufferBuilder::processBackgroundSection(const std::byte* gameMemory, EntitySection& section, uint64_t index)
	{
		for (uint64_t i = 0; i < section.descriptorCount; ++i)
		{
			const EntityReadDescriptor* readDescriptor = section.readDescriptors + i;

			const auto* renderEntity = reinterpret_cast<const ZeroXBridge::RenderEntity*>(
				gameMemory + readDescriptor->start + readDescriptor->offset);

			addCommand<CommandType::UPDATE_ENTITY_TRANSFORM>(renderer::UpdateTransformCommand{
				.transform		= renderEntity->transform,
				.batchIndex		= index,
				.entityIndex	= i
			});

			addCommand<CommandType::UPDATE_ENTITY_MATERIAL>(renderer::UpdateMaterialCommand{
				.material		= renderEntity->material,
				.batchIndex		= index,
				.entityIndex	= i
			});

			renderer::SetTextureLayerCommand setTextureLayerParams;
			setTextureLayerParams.batchIndex	= index;
			setTextureLayerParams.entityIndex	= i;
			setTextureLayerParams.layer			= renderEntity->layer;

			addCommand<CommandType::SET_TEXTURE_LAYER>(setTextureLayerParams);
		}
	}

	void CommandBufferBuilder::processSpriteSection(const std::byte* gameMemory, EntitySection& section, uint64_t index)
	{
		for (uint64_t i = 0; i < section.descriptorCount; ++i)
		{
			const EntityReadDescriptor* readDescriptor = section.readDescriptors + i;

			const auto* renderEntity = reinterpret_cast<const ZeroXBridge::RenderEntity*>(gameMemory + readDescriptor->start + readDescriptor->offset);

			addCommand<CommandType::UPDATE_ENTITY_TRANSFORM>(renderer::UpdateTransformCommand{
				.transform		= renderEntity->transform,
				.batchIndex		= index,
				.entityIndex	= i
			});

			addCommand<CommandType::UPDATE_ENTITY_MATERIAL>(renderer::UpdateMaterialCommand{
				.material		= renderEntity->material,
				.batchIndex		= index,
				.entityIndex	= i
			});
		}
	}
}