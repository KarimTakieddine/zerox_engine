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
		UNKNOWN			= 0,
		UNLIT_COLOR		= 1,
		BACKGROUND		= 2,
		SPRITE			= 3,
		TILED_SPRITE	= 4,
		COLLIDER		= 5,
		LAST			= COLLIDER
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

		void mapMesh(int index, MeshType type);
		void mapShaderProgram(int index, ShaderProgramType type);
		void mapVertexArray(int index, VertexArrayType type);

		int getMeshIndex(MeshType type) const;
		int getShaderProgramIndex(ShaderProgramType type) const;
		int getVertexArrayIndex(VertexArrayType type) const;

	private:
		std::vector<int> m_meshMappings;
		std::vector<int> m_shaderProgramMappings;
		std::vector<int> m_vertexArrayMappings;
	};
}
