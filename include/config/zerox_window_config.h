#pragma once

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
}
