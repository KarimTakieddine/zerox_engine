#pragma once

#include <functional>
#include <string>

namespace ZeroX
{
    struct WindowConfig
    {
        std::string name        { "Unnamed" };
        int width               { 0 };
        int height              { 0 };
        int openGLVersionMajor  { 0 };
        int openGLVersionMinor  { 0 };
        bool fullscreen         { false };
        bool resizeable         { false };
    };

    using ResizeCallback = std::function<void(int, int)>;

    class IWindow
    {
    public:
        struct Size
        {
            int width   { 0 };
            int height  { 0 };
        };

        virtual ~IWindow() = default;
        virtual void setSize(int width, int height) = 0;
        virtual Size getSize() const = 0;
        virtual bool shouldClose() = 0;
        virtual void close() = 0;
        virtual void processEvents() = 0;
        virtual void swapBuffers() = 0;
        virtual void bindGraphicsContext() = 0;
        virtual void unbindGraphicsContext() = 0;
        virtual void setResizeCallback(ResizeCallback resizeCallback) = 0;
        virtual ResizeCallback getResizeCallback() const = 0;
    };
}
