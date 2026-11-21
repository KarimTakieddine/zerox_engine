#include <stdexcept>

#include "config/zerox_graphics_config.h"

namespace ZeroX
{
	void GraphicsConfig::setMeshCount(size_t count)
	{
		m_meshes.reserve(count);
	}

	void GraphicsConfig::addMesh(const MeshConfig& config)
	{
		m_meshes.emplace_back(config);
	}

	const MeshConfig& GraphicsConfig::getMesh(size_t index) const
	{
		if (index >= m_meshes.size())
		{
			throw std::runtime_error(
				std::string("Failed to get MeshConfig. Out of bounds: ")
				+ std::to_string(index));
		}

		return m_meshes.at(index);
	}

	const std::vector<MeshConfig>& GraphicsConfig::getMeshes() const
	{
		return m_meshes;
	}

	void GraphicsConfig::setTextureCount(size_t count)
	{
		m_textures.reserve(count);
	}

	void GraphicsConfig::addTexture(const renderer::Texture& texture)
	{
		m_textures.emplace_back(texture);
	}

	const renderer::Texture& GraphicsConfig::getTexture(size_t index) const
	{
		if (index >= m_textures.size())
		{
			throw std::runtime_error(
				std::string("Failed to get Texture. Out of bounds: ")
				+ std::to_string(index));
		}

		return m_textures.at(index);
	}

	const std::vector<renderer::Texture>& GraphicsConfig::getTextures() const
	{
		return m_textures;
	}

	void GraphicsConfig::setTextureArrayCount(size_t count)
	{
		m_textureArrays.reserve(count);
	}

	void GraphicsConfig::addTextureArray(const TextureArrayConfig& config)
	{
		m_textureArrays.emplace_back(config);
	}

	const TextureArrayConfig& GraphicsConfig::getTextureArray(size_t index) const
	{
		if (index >= m_textureArrays.size())
		{
			throw std::runtime_error(
				std::string("Failed to get TextureArray. Out of bounds: ")
				+ std::to_string(index));
		}

		return m_textureArrays.at(index);
	}

	const std::vector<TextureArrayConfig>& GraphicsConfig::getTextureArrays() const
	{
		return m_textureArrays;
	}

	void GraphicsConfig::setCameraUniformNames(const std::vector<const char*>& names)
	{
		m_cameraUniformNames = names;
	}
	
	const std::vector<const char*>& GraphicsConfig::getCameraUniformNames() const
	{
		return m_cameraUniformNames;
	}

	const std::string& GraphicsConfig::getCameraUniformBuffer() const
	{
		return m_cameraUniformBuffer;
	}

	void GraphicsConfig::setCameraUniformBuffer(const std::string& name)
	{
		m_cameraUniformBuffer = name;
	}

	const renderer::Eye& GraphicsConfig::getCameraEye() const
	{
		return m_cameraEye;
	}

	void GraphicsConfig::setCameraEye(const renderer::Eye& eye)
	{
		m_cameraEye = eye;
	}

	const renderer::Frustum& GraphicsConfig::getCameraFrustum() const
	{
		return m_cameraFrustum;
	}

	void GraphicsConfig::setCameraFrustum(const renderer::Frustum& frustum)
	{
		m_cameraFrustum = frustum;
	}

	void GraphicsConfig::setVertexArrayCount(size_t count)
	{
		m_vertexArrayCount = count;
	}

	size_t GraphicsConfig::getVertexArrayCount() const
	{
		return m_vertexArrayCount;
	}
}