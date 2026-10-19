#pragma once

#include <cstdint>

#include <string>
#include <vector>

namespace ZeroX
{
    enum class SoundType : uint8_t
    {
        UNKNOWN         = 0,
        SOUND_EFFECT    = 1,
        BGM             = 2
    };

    struct AudioSound
    {
        std::string path;
        SoundType type{ SoundType::UNKNOWN };
    };

    struct AudioConfig
    {
        std::vector<AudioSound> sounds;
    };
}
