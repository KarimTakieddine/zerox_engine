#include <stdexcept>

#include "zerox_glfw_window.h"

namespace
{
    void onWindowResize(GLFWwindow* window, int width, int height)
    {
        auto* instance = static_cast<ZeroX::GLFWWindow*>(glfwGetWindowUserPointer(window));

        instance->getResizeCallback()(width, height);
    }
}

namespace ZeroX
{
    GLFWWindow::GLFWWindow(const WindowConfig& config)
    {
        const int width     = static_cast<int>(config.width);
        const int height    = static_cast<int>(config.height);

        if (width == 0 || height == 0)
            throw std::invalid_argument("Failed to initialize window. Width and/or height is configured incorrectly");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, static_cast<int>(config.openGLVersionMajor));
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, static_cast<int>(config.openGLVersionMinor));
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, config.resizeable ? GL_TRUE : GL_FALSE);

        m_handle = glfwCreateWindow(
            width,
            height,
            config.name.c_str(),
            config.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
        
        if (!m_handle)
        {
            const char* description;
            glfwGetError(&description);
            throw std::runtime_error("Failed to initialize window. GLFW error: " + std::string(description));
        }

        glfwSetWindowUserPointer(m_handle, this);
        glfwSetWindowSizeCallback(m_handle, ::onWindowResize);
        glfwMakeContextCurrent(m_handle);
    }

    GLFWWindow::~GLFWWindow()
    {
        glfwDestroyWindow(m_handle);
    }

    void GLFWWindow::setSize(int width, int height)
    {
        glfwSetWindowSize(m_handle, width, height);
    }

    IWindow::Size GLFWWindow::getSize() const
    {
        Size s;

        glfwGetWindowSize(m_handle, &s.width, &s.height);

        return s;
    }

    bool GLFWWindow::shouldClose()
    {
        return glfwWindowShouldClose(m_handle);
    }

    void GLFWWindow::close()
    {
        glfwSetWindowShouldClose(m_handle, GLFW_TRUE);
    }

    void GLFWWindow::processEvents()
    {
        glfwPollEvents();
    }

    void GLFWWindow::swapBuffers()
    {
        glfwSwapBuffers(m_handle);
    }

    void GLFWWindow::bindGraphicsContext()
    {
        glfwMakeContextCurrent(m_handle);
    }

    void GLFWWindow::unbindGraphicsContext()
    {
        glfwMakeContextCurrent(nullptr);
    }

    void GLFWWindow::setResizeCallback(ResizeCallback resizeCallback)
    {
        m_resizeCallback = resizeCallback;
    }

    ResizeCallback GLFWWindow::getResizeCallback() const
    {
        return m_resizeCallback;
    }
}