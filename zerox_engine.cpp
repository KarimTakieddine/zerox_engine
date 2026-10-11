#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <renderer.hpp>
#include <render_frame_buffer.h>
#include <mesh.hpp>
#include <shader.h>

#include "game_library_watcher.h"
#include "zerox_engine_logger.h"
#include "zerox_engine_runtime.h"
#include "zerox_glfw_window.h"

int main(int argc, char** argv)
{
    std::unique_ptr<ZeroX::EngineLogger> engineLogger = std::make_unique<ZeroX::EngineLogger>();
    ZeroX::globalEngineLogger = engineLogger.get();
    engineLogger->initialize();

    // TODO(Karim): Do we need heap memory here honestly?

    try
    {
        ZeroX::WindowConfig windowConfig = {
            .name               = "ZeroX Engine",
            .width              = 1920,
            .height             = 1080,
            .openGLVersionMajor = 4,
            .openGLVersionMinor = 6,
            .fullscreen         = false,
            .resizeable         = true
        };

        std::unique_ptr<ZeroX::EngineRuntime> engineRuntime = std::make_unique<ZeroX::EngineRuntime>(windowConfig);

        engineRuntime->run();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Engine runtime initialization failure: " << e.what();
    }

    engineLogger->processMessages();
    engineLogger->terminate();

    // if (!glfwInit()) { return 1; }

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    // GLFWwindow * window = glfwCreateWindow(1920, 1080, "Test", nullptr, nullptr);
    // glfwMakeContextCurrent(window);

    // while (!glfwWindowShouldClose(window))
    // {
    //     glfwPollEvents();

    //     glfwSwapBuffers(window);
    // }

    // glfwDestroyWindow(window);
    // glfwTerminate();

    return 0;
}