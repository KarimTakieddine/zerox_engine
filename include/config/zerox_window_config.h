#pragma once

#include <string>

namespace ZeroX
{
	struct WindowConfig
    {
        static constexpr unsigned int DEFAULT_WIDTH                 = 1920;
        static constexpr unsigned int DEFAULT_HEIGHT                = 1080;
        static constexpr const char* DEFAULT_NAME                   = "UnnamedWindow";
        static constexpr unsigned int DEFAULT_OPENGL_VERSION_MAJOR  = 4;
        static constexpr unsigned int DEFAULT_OPENGL_VERSION_MINOR  = 6;
        static constexpr bool DEFAULT_RESIZEABLE                    = false;
        static constexpr bool DEFAULT_FULLSCREEN                    = false;

        std::string name                { DEFAULT_NAME };
        unsigned int width              { DEFAULT_WIDTH };
        unsigned int height             { DEFAULT_HEIGHT };
        unsigned int openGLVersionMajor { DEFAULT_OPENGL_VERSION_MAJOR };
        unsigned int openGLVersionMinor { DEFAULT_OPENGL_VERSION_MINOR };
        bool resizeable                 { DEFAULT_RESIZEABLE };
        bool fullscreen                 { DEFAULT_FULLSCREEN };
    };
}
