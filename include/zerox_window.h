#pragma once

#include <functional>
#include <string>

namespace ZeroX
{
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
