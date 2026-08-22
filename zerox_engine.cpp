#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <renderer.hpp>
#include <render_frame_buffer.h>
#include <mesh.hpp>
#include <shader.h>

#include "game_library_watcher.h"

#include <iostream>

std::atomic<bool> shouldRun{true};

void monitor_file_changes()
{
    ZeroX::GameLibraryWatcher gameLibraryWatcher;

    uint64_t fileChangeCounter = 0;

    if (!gameLibraryWatcher.startWatching("/home/mouns/Repositories/zerox_game/build", "libzerox_game.so"))
    {
        return;
    }

    while (shouldRun.load(std::memory_order_acquire))
    {
        const uint64_t watchCounter = gameLibraryWatcher.getChangeCounter();

        if (fileChangeCounter != watchCounter)
        {
            std::cout << "File has changed" << std::endl;

            fileChangeCounter = watchCounter;
        }
    }

    gameLibraryWatcher.stopWatching();
}

int main(int argc, char** argv)
{
    std::thread gameThread(monitor_file_changes);

    renderer::FrameBuffer renderFrameBuffer;
    renderFrameBuffer.allocate(64);

    renderer::PlatformFunctions platformFunctions {
        .getFileSize = &platform::getFileSize,
        .readFile = &platform::readFile };

    renderer::Vertex quadVertices[4] = {
        { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
    };

    unsigned int quadTriangles[6] = { 0, 2, 1, 0, 3, 2 };

    renderer::Mesh meshes[1] = { { quadVertices, quadTriangles, 4, 6 } };

    renderer::Shader shaderList[2] = {
        { "./shaders/3d_transform_vertex.slh", renderer::Shader::Type::VERTEX },
        { "./shaders/3d_transform_fragment.slh", renderer::Shader::Type::FRAGMENT },
    };

    renderer::Eye cameraEye = {
        .position   = glm::vec3{ 0.0f, 0.0f, 10.0f },
        .target	    = glm::vec3{ 0.0f, 0.0f, 0.0f },
        .up		    = glm::vec3{ 0.0f, 1.0f, 0.0f }
    };

    renderer::Frustum cameraFrustum = {
        .fov    = 45.0f,
        .aspect = 1920.0f / 1080.0f,
        .near   = 1.0f,
        .far    = 100.0f
    };

    const char* cameraUniformNames[4] = {
        "cameraProjection",
        "cameraLocalToWorld",
        "cameraLocalRotation",
        "cameraView"
    };

    const size_t entityCount = 4;

    size_t shaderIndices[2] = { 0, 1 };

    renderer::ShaderCompileStep shaderCompileSteps[1] = {
        { .shaderIndices = shaderIndices, .shaderCount = 2, .programIndex = 0 }
    };

    renderer::ShaderLocationsLink shaderLocationsLinks[1] = {
        { .locationsIndex = 0, .shaderProgramIndex = 0 }
    };

    renderer::RenderBatchConfig renderBatchConfigs[1] = {
        {
            .batchIndex = 0,
            .shaderProgramIndex = 0,
            .vertexArrayIndex = 0,
            .shaderLocationsIndex = 0,
            .meshIndex = 0
        }
    };

    renderer::GraphicsConfig graphicsConfig = {
        .meshes = meshes,
        .textures = nullptr,
        .renderEntityCounts = &entityCount,
        .shaders = shaderList,
        .cameraEye = &cameraEye,
        .cameraFrustum = &cameraFrustum,
        .cameraUniformBuffer = "CameraMatrices",
        .cameraUniformNames = cameraUniformNames,
        .shaderCompileSteps = shaderCompileSteps,
        .shaderLocationsLinks = shaderLocationsLinks,
        .renderBatchConfigs = renderBatchConfigs,
        .meshCount = 1,
        .bufferCount = 3,
        .vertexArrayCount = 1,
        .textureCount = 0,
        .shaderCount = 2,
        .shaderProgramCount = 1,
        .locationsDescriptorCount = 1,
        .renderBatchCount = 1,
        .compileStepCount = 1,
        .locationsLinkCount = 1,
        .batchConfigCount = 1
    };

    if (glfwInit() != GLFW_TRUE)
    {
        return 2;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Minimal Renderer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();

        return 3;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        return 4;
    }

    renderer::Transform transform;
    transform.localToWorld[3].x = 2.5f;

    renderer::Command updateTransformCommand{ .type = renderer::CommandType::UPDATE_ENTITY_TRANSFORM };

    size_t batchIndex   = 0;
    size_t entityIndex  = 0;

    std::memcpy(updateTransformCommand.data, &batchIndex, sizeof(size_t));
    std::memcpy(updateTransformCommand.data + sizeof(size_t), &entityIndex, sizeof(size_t));
    std::memcpy(updateTransformCommand.data + 2 * sizeof(size_t), &transform, sizeof(renderer::Transform));

    renderFrameBuffer.push({ { updateTransformCommand } });

    renderer::Allocator renderAllocator;
    renderer::allocateGraphicsResources(&renderAllocator, &graphicsConfig);
    renderer::MutableGraphicsMemory mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
    renderer::initializeGraphicsResources(mutableGraphicsMemory, &graphicsConfig, &platformFunctions);
    renderer::initializeGraphicsState();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        renderer::Frame renderFrame;
        if (renderFrameBuffer.pop(renderFrame))
        {
            renderer::processFrame(mutableGraphicsMemory, &renderFrame);
        }

        renderer::clearFrameBuffer();
        renderer::render(renderer::freezeGraphicsMemory(mutableGraphicsMemory));

        glfwSwapBuffers(window);
    }

    renderer::freeGraphicsResources(mutableGraphicsMemory);
    
    glfwDestroyWindow(window);
    glfwTerminate();

    shouldRun.store(false, std::memory_order_release);

    if (gameThread.joinable())
    {
        gameThread.join();
    }

    return 0;
}