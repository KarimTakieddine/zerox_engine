#pragma once

#include <string>
#include <vector>

#include <eye.h>
#include <frustum.h>
#include <renderer.hpp>
#include <shader.h>
#include <texture.h>

namespace ZeroX
{
	struct ShaderProgramConfig
	{
		std::vector<renderer::Shader> shaders;
	};

	struct MeshConfig
	{
		std::vector<renderer::Vertex> vertices;
		std::vector<unsigned int> triangles;
	};

	struct ShaderCompileConfig
	{
		std::vector<size_t> shaderIndices	{ 0 };
		size_t programIndex					{ 0 };
	};

	class GraphicsConfig
	{
	public:
		void setMeshCount(size_t count);
		void addMesh(const MeshConfig& config);
		const MeshConfig& getMesh(size_t index) const;
		const std::vector<MeshConfig>& getMeshes() const;

		void setShaderProgramCount(size_t count);
		void addShaderProgram(const ShaderProgramConfig& config);
		const ShaderProgramConfig& getShaderProgram(size_t index) const;
		const std::vector<ShaderProgramConfig>& getShaderPrograms() const;

		void setTextureCount(size_t count);
		void addTexture(const renderer::Texture& texture);
		const renderer::Texture& getTexture(size_t index) const;
		const std::vector<renderer::Texture>& getTextures() const;

		void setCameraUniformNames(const std::vector<const char*>& names);
		const std::vector<const char*>& getCameraUniformNames() const;

		void setShaderLocationsLinks(const std::vector<renderer::ShaderLocationsLink>& links);
		const std::vector<renderer::ShaderLocationsLink>& getShaderLocationsLinks() const;

		const std::string& getCameraUniformBuffer() const;
		void setCameraUniformBuffer(const std::string& name);

		const renderer::Eye& getCameraEye() const;
		void setCameraEye(const renderer::Eye& eye);

		const renderer::Frustum& getCameraFrustum() const;
		void setCameraFrustum(const renderer::Frustum& frustum);

		void setVertexArrayCount(size_t count);
		size_t getVertexArrayCount() const;

		std::vector<ShaderCompileConfig> getShaderCompileSteps() const;

	private:
		std::vector<MeshConfig> m_meshes;
		std::vector<ShaderProgramConfig> m_shaderPrograms;
		std::vector<renderer::Texture> m_textures;
		std::vector<renderer::ShaderLocationsLink> m_locationsLinks;
		std::vector<const char*> m_cameraUniformNames;
		std::string m_cameraUniformBuffer;
		renderer::Eye m_cameraEye;
		renderer::Frustum m_cameraFrustum;
		size_t m_vertexArrayCount{ 0 };
	};
}
