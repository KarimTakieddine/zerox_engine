#include <stdexcept>
#include <string>

#include "zerox_memory_map.h"

namespace ZeroX
{
	void MemoryMap::mapMesh(size_t index, MeshType type)
	{
		if (type == MeshType::UNKNOWN)
		{
			throw std::runtime_error(
				std::string("Failed to map mesh. Unknown mesh type: ") +
				std::to_string(static_cast<uint8_t>(type)));
		}

		m_meshMappings.emplace(std::make_pair(type, index));
	}

	void MemoryMap::mapShaderProgram(size_t index, ShaderProgramType type)
	{
		if (type == ShaderProgramType::UNKNOWN)
		{
			throw std::runtime_error(
				std::string("Failed to map shader program. Unknown shader program type: ") +
				std::to_string(static_cast<uint8_t>(type)));
		}

		m_shaderProgramMappings.emplace(std::make_pair(type, index));
	}

	void MemoryMap::mapVertexArray(size_t index, VertexArrayType type)
	{
		if (type == VertexArrayType::UNKNOWN)
		{
			throw std::runtime_error(
				std::string("Failed to map vertex array. Unknown vertex array type: ") +
				std::to_string(static_cast<uint8_t>(type)));
		}

		m_vertexArrayMappings.emplace(std::make_pair(type, index));
	}

	void MemoryMap::mapLocationsDescriptor(size_t index, ShaderProgramType type)
	{
		if (type == ShaderProgramType::UNKNOWN)
		{
			throw std::runtime_error(
				std::string("Failed to map locations descriptor. Unknown shader program type: ") +
				std::to_string(static_cast<uint8_t>(type)));
		}

		m_locationsDescriptorMappings.emplace(std::make_pair(type, index));
	}

	size_t MemoryMap::getMeshIndex(MeshType type) const
	{
		const auto it = m_meshMappings.find(type);

		if (it == m_meshMappings.cend())
		{
			throw std::runtime_error(
				std::string("Could not find mesh index for type: ")
				+ std::to_string(static_cast<uint8_t>(type)));
		}

		return it->second;
	}

	size_t MemoryMap::getShaderProgramIndex(ShaderProgramType type) const
	{
		const auto it = m_shaderProgramMappings.find(type);

		if (it == m_shaderProgramMappings.cend())
		{
			throw std::runtime_error(
				std::string("Could not find shader program index for type: ")
				+ std::to_string(static_cast<uint8_t>(type)));
		}

		return it->second;
	}

	size_t MemoryMap::getVertexArrayIndex(VertexArrayType type) const
	{
		const auto it = m_vertexArrayMappings.find(type);

		if (it == m_vertexArrayMappings.cend())
		{
			throw std::runtime_error(
				std::string("Could not find vertex array index for type: ")
				+ std::to_string(static_cast<uint8_t>(type)));
		}

		return it->second;
	}

	size_t MemoryMap::getLocationsDescriptorIndex(ShaderProgramType type) const
	{
		const auto it = m_locationsDescriptorMappings.find(type);

		if (it == m_locationsDescriptorMappings.cend())
		{
			throw std::runtime_error(
				std::string("Could not find locations descriptor index for type: ")
				+ std::to_string(static_cast<uint8_t>(type)));
		}

		return it->second;
	}
}