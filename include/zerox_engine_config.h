#pragma once

#include <cstdint>

namespace ZeroX
{
    struct GameConfig
    {
        const char* gameLibraryDir  { nullptr };
        const char* gameLibraryFile { nullptr };
        uint8_t maxQueuedFrames     { 4 };
    };
}
