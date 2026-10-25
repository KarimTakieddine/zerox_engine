#include <stdexcept>

#include "config/zerox_config_loader.h"
#include "config/zerox_config_parser.h"

namespace ZeroX
{
	std::future<WindowConfig> loadWindowConfig(const pugi::xml_node& node)
	{
		return std::async(
			std::launch::async, [node]() -> WindowConfig {

				WindowConfig result = { };
				if (node)
				{
					result = parseWindowConfig(node);

					setReasonableDefaults(result);
				}

				return result;
			}
		);
	}

	std::future<AudioConfig> loadAudioConfig(const pugi::xml_node& node)
	{
		return std::async(
			std::launch::async, [node]() -> AudioConfig {

				AudioConfig result = { };
				if (node)
				{
					result = parseAudioConfig(node);

					// TODO(Karim): Implement: setReasonableDefaults(result);
				}

				return result;
			}
		);
	}

	std::future<GameConfig> loadGameConfig(const pugi::xml_node& node)
	{
		return std::async(
			std::launch::async, [node]() -> GameConfig {

				GameConfig result = { };
				if (node)
				{
					result = parseGameConfig(node);

					setReasonableDefaults(result);
				}

				return result;
			}
		);
	}

	std::future<std::pair<GraphicsConfig, MemoryMap>> loadGraphicsConfig()
	{
		return std::async(
			std::launch::async, []() -> std::pair<GraphicsConfig, MemoryMap> {

				GraphicsConfig config;

				config.setMeshCount(1);

				MeshConfig quadMesh = {
					.vertices = {
						{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
						{ { -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
						{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
						{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
					},
					.triangles = { 0, 2, 1, 0, 3, 2 }
				};

				MeshConfig colliderMesh = {
					.vertices = {
						{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
						{ { -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
						{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
						{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
					},
					.triangles = { 0, 1, 1, 2, 2, 3, 3, 0 }
				};

				config.addMesh(quadMesh);
				config.addMesh(colliderMesh);

				config.setVertexArrayCount(2);

				config.setShaderProgramCount(3);

				ShaderProgramConfig unlitColorProgram = {
					.shaders = {
						{ "./shaders/unlit_color/vertex.slh", renderer::Shader::Type::VERTEX },
						{ "./shaders/unlit_color/fragment.slh", renderer::Shader::Type::FRAGMENT }
					}
				};

				ShaderProgramConfig spriteProgram = {
					.shaders = {
						{ "./shaders/sprite/vertex.slh", renderer::Shader::Type::VERTEX },
						{ "./shaders/sprite/fragment.slh", renderer::Shader::Type::FRAGMENT }
					}
				};

				ShaderProgramConfig colliderProgram = {
					.shaders = {
						{ "./shaders/collider/vertex.slh", renderer::Shader::Type::VERTEX },
						{ "./shaders/collider/fragment.slh", renderer::Shader::Type::FRAGMENT }
					}
				};

				config.addShaderProgram(unlitColorProgram);
				config.addShaderProgram(spriteProgram);
				config.addShaderProgram(colliderProgram);

				config.setCameraEye({
					.position	= glm::vec3{ 0.0f, 0.0f, 10.0f },
					.target		= glm::vec3{ 0.0f, 0.0f, 0.0f },
					.up			= glm::vec3{ 0.0f, 1.0f, 0.0f }
				});

				config.setCameraFrustum({
					.fov        = 80.0f,
					.aspect     = 1920.0f / 1080.0f,
					.nearPlane  = 1.0f,
					.farPlane   = 100.0f
				});

				config.setCameraUniformNames({
					"cameraProjection",
					"cameraLocalToWorld",
					"cameraLocalRotation",
					"cameraView"
				});

				config.setCameraUniformBuffer("CameraMatrices");

				config.setShaderLocationsLinks({
					{ .locationsIndex = 0, .shaderProgramIndex = 0 },
					{ .locationsIndex = 1, .shaderProgramIndex = 1 },
					{ .locationsIndex = 2, .shaderProgramIndex = 2 }
				});

				renderer::Texture megaTexture = {
					.path		= "./textures/the_mega_texture.png",
					.wrapModeS	= renderer::WrapMode::CLAMP_TO_EDGE,
					.wrapModeT	= renderer::WrapMode::CLAMP_TO_EDGE,
					.minFilter	= renderer::Filter::NEAREST,
					.magFilter	= renderer::Filter::NEAREST
				};

				config.addTexture(megaTexture);

				MemoryMap memoryMap;

				memoryMap.mapMesh(0, MeshType::QUAD);
				memoryMap.mapShaderProgram(1, ShaderProgramType::SPRITE);
				memoryMap.mapVertexArray(0, VertexArrayType::TEXTURED_QUAD);
				memoryMap.mapLocationsDescriptor(1, ShaderProgramType::SPRITE);

				memoryMap.mapMesh(1, MeshType::COLLIDER);
				memoryMap.mapShaderProgram(2, ShaderProgramType::COLLIDER);
				memoryMap.mapVertexArray(1, VertexArrayType::COLLIDER);
				memoryMap.mapLocationsDescriptor(2, ShaderProgramType::COLLIDER);

				return std::make_pair(config, memoryMap);
			}
		);
	}
}