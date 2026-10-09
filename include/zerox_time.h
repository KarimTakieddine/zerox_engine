#pragma once

#include <cstdint>

namespace ZeroX
{
    struct Time
    {
        double deltaTimeSeconds     { 0.0 };
        double smoothDeltaTime      { 0.0 };
        uint64_t renderFrameIndex   { 0 };
        uint64_t gameFrameIndex     { 0 };
    };
}
