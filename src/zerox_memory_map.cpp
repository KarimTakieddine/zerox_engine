#include <stdexcept>
#include <string>

#include "zerox_memory_map.h"

namespace ZeroX
{
	MemoryMap::MemoryMap() :
		m_meshMappings			(static_cast<size_t>(MeshType::LAST) + 1, 0),
		m_shaderProgramMappings	(static_cast<size_t>(ShaderProgramType::LAST) + 1, 0),
		m_descriptorMappings	(static_cast<size_t>(ShaderProgramType::LAST) + 1, 0),
		m_vertexArrayMappings	(static_cast<size_t>(VertexArrayType::LAST) + 1, 0) { }

	void MemoryMap::mapMesh(uint64_t index, MeshType type)
	{
		m_meshMappings[static_cast<size_t>(type)] = index;
	}

	void MemoryMap::mapShaderProgram(uint64_t index, ShaderProgramType type)
	{
		m_shaderProgramMappings[static_cast<size_t>(type)] = index;
	}

	void MemoryMap::mapVertexArray(uint64_t index, VertexArrayType type)
	{
		m_vertexArrayMappings[static_cast<size_t>(type)] = index;
	}

	void MemoryMap::mapLocationsDescriptor(uint64_t index, ShaderProgramType type)
	{
		m_descriptorMappings[static_cast<size_t>(type)] = index;
	}

	uint64_t MemoryMap::getMeshIndex(MeshType type) const
	{
		return m_meshMappings[static_cast<size_t>(type)];
	}

	uint64_t MemoryMap::getShaderProgramIndex(ShaderProgramType type) const
	{
		return m_shaderProgramMappings[static_cast<size_t>(type)];
	}

	uint64_t MemoryMap::getVertexArrayIndex(VertexArrayType type) const
	{
		return m_vertexArrayMappings[static_cast<size_t>(type)];
	}

	uint64_t MemoryMap::getLocationsDescriptorIndex(ShaderProgramType type) const
	{
		return m_descriptorMappings[static_cast<size_t>(type)];
	}
}