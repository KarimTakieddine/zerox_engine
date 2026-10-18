#pragma once

#include <GLFW/glfw3.h>

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

    private:
        GLFWwindow* m_handle            { nullptr };
        ResizeCallback m_resizeCallback { nullptr };
    };
}
