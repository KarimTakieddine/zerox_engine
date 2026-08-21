#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "platform.h"
#include "renderer.hpp"

int main(int argc, char** argv)
{
    renderer::PlatformFunctions platformFunctions {
        .getFileSize = &platform::getFileSize,
        .readFile = &platform::readFile };

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

    renderer::Allocator renderAllocator;
    renderer::allocate(&renderAllocator);
    renderer::MutableGraphicsMemory mutableGraphicsMemory = renderer::readGraphicsMemory(&renderAllocator);
    renderer::initializeGraphicsResources(mutableGraphicsMemory, &platformFunctions);
    renderer::initializeGraphicsState();

    renderer::ConstGraphicsMemory constGraphicsMemory = renderer::freezeGraphicsMemory(mutableGraphicsMemory);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        renderer::clearFrameBuffer();
        renderer::render(constGraphicsMemory);

        glfwSwapBuffers(window);
    }

    renderer::freeGraphicsResources(mutableGraphicsMemory);
    
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}