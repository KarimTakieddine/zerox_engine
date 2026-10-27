#pragma once

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "config/zerox_window_config.h"
#include "zerox_window.h"

namespace ZeroX
{
    class GLFWWindow : public IWindow
    {
    public:
        GLFWWindow(const WindowConfig& config);
        ~GLFWWindow() override;

        GLFWWindow(GLFWWindow&) = delete;
        GLFWWindow& operator=(GLFWWindow&) = delete;

        void setSize(int width, int height) override;
        Size getSize() const override;
        bool shouldClose() override;
        void close() override;
        void processEvents() override;
        void swapBuffers() override;
        void bindGraphicsContext() override;
        void unbindGraphicsContext() override;
        void setResizeCallback(ResizeCallback resizeCallback) override;
        ResizeCallback getResizeCallback() const override;

#ifdef _WIN32
        HWND getNativeWin32Handle() const;
#endif

    private:
        GLFWwindow* m_handle            { nullptr };
        ResizeCallback m_resizeCallback { nullptr };
    };
}
