#pragma once

#include <cstdint>
#include <vector>

namespace ZeroX
{
	enum class MeshType : uint8_t
	{
		UNKNOWN		= 0,
		QUAD		= 1,
		COLLIDER	= 2,
		LAST		= COLLIDER
	};

	enum class ShaderProgramType : uint8_t
	{
		UNKNOWN		= 0,
		UNLIT_COLOR	= 1,
		BACKGROUND	= 2,
		SPRITE		= 3,
		COLLIDER	= 4,
		LAST		= COLLIDER
	};

	enum class VertexArrayType : uint8_t
	{
		UNKNOWN			= 0,
		TEXTURED_QUAD	= 1,
		COLLIDER		= 2,
		LAST			= COLLIDER
	};

	class MemoryMap
	{
	public:
		MemoryMap();

		void mapMesh(uint64_t index, MeshType type);
		void mapShaderProgram(uint64_t index, ShaderProgramType type);
		void mapVertexArray(uint64_t index, VertexArrayType type);
		void mapLocationsDescriptor(uint64_t index, ShaderProgramType type);

		uint64_t getMeshIndex(MeshType type) const;
		uint64_t getShaderProgramIndex(ShaderProgramType type) const;
		uint64_t getVertexArrayIndex(VertexArrayType type) const;
		uint64_t getLocationsDescriptorIndex(ShaderProgramType type) const;

	private:
		std::vector<uint64_t> m_meshMappings;
		std::vector<uint64_t> m_shaderProgramMappings;
		std::vector<uint64_t> m_descriptorMappings;
		std::vector<uint64_t> m_vertexArrayMappings;
	};
}
