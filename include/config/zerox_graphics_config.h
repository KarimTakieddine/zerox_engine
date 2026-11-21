#pragma once

#include <string>
#include <vector>

#include <renderer.hpp>
#include <shader.h>

namespace ZeroX
{
	struct MeshConfig
	{
		std::vector<renderer::Vertex> vertices;
		std::vector<unsigned int> triangles;
	};

	struct TextureArrayConfig
	{
		std::vector<const char*> textureFiles	{ };
		ZeroXBridge::Texture data				{ };
	};

	class GraphicsConfig
	{
	public:
		void setMeshCount(size_t count);
		void addMesh(const MeshConfig& config);
		const MeshConfig& getMesh(size_t index) const;
		const std::vector<MeshConfig>& getMeshes() const;

		void setTextureCount(size_t count);
		void addTexture(const renderer::Texture& texture);
		const renderer::Texture& getTexture(size_t index) const;
		const std::vector<renderer::Texture>& getTextures() const;

		void setTextureArrayCount(size_t count);
		void addTextureArray(const TextureArrayConfig& config);
		const TextureArrayConfig& getTextureArray(size_t index) const;
		const std::vector<TextureArrayConfig>& getTextureArrays() const;

		void setCameraUniformNames(const std::vector<const char*>& names);
		const std::vector<const char*>& getCameraUniformNames() const;

		const std::string& getCameraUniformBuffer() const;
		void setCameraUniformBuffer(const std::string& name);

		const renderer::Eye& getCameraEye() const;
		void setCameraEye(const renderer::Eye& eye);

		const renderer::Frustum& getCameraFrustum() const;
		void setCameraFrustum(const renderer::Frustum& frustum);

		void setVertexArrayCount(size_t count);
		size_t getVertexArrayCount() const;

	private:
		std::vector<MeshConfig> m_meshes;
		std::vector<renderer::Texture> m_textures;
		std::vector<TextureArrayConfig> m_textureArrays;
		std::vector<const char*> m_cameraUniformNames;
		std::string m_cameraUniformBuffer;
		renderer::Eye m_cameraEye;
		renderer::Frustum m_cameraFrustum;
		size_t m_vertexArrayCount{ 0 };
	};
}
