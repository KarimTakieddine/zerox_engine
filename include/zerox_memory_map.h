#pragma once

#include <cstdint>
#include <unordered_map>

namespace ZeroX
{
	enum class MeshType : uint8_t
	{
		UNKNOWN		= 0,
		QUAD		= 1,
		COLLIDER	= 2
	};

	enum class ShaderProgramType : uint8_t
	{
		UNKNOWN		= 0,
		UNLIT_COLOR	= 1,
		BACKGROUND	= 2,
		SPRITE		= 3,
		COLLIDER	= 4
	};

	enum class VertexArrayType : uint8_t
	{
		UNKNOWN			= 0,
		TEXTURED_QUAD	= 1,
		BACKGROUND		= 2,
		COLLIDER		= 3
	};

	class MemoryMap
	{
	public:
		void mapMesh(size_t index, MeshType type);
		void mapShaderProgram(size_t index, ShaderProgramType type);
		void mapVertexArray(size_t index, VertexArrayType type);
		void mapLocationsDescriptor(size_t index, ShaderProgramType type);

		size_t getMeshIndex(MeshType type) const;
		size_t getShaderProgramIndex(ShaderProgramType type) const;
		size_t getVertexArrayIndex(VertexArrayType type) const;
		size_t getLocationsDescriptorIndex(ShaderProgramType type) const;

	private:
		std::unordered_map<MeshType, size_t> m_meshMappings;
		std::unordered_map<ShaderProgramType, size_t> m_shaderProgramMappings;
		std::unordered_map<ShaderProgramType, size_t> m_locationsDescriptorMappings;
		std::unordered_map<VertexArrayType, size_t> m_vertexArrayMappings;
	};
}
