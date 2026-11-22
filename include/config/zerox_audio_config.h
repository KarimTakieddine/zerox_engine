#pragma once

#include <cstdint>

#include <string>
#include <vector>

#include <zerox_audio.h>

namespace ZeroX
{
    struct AudioFile
    {
        std::string path;
        ZeroXBridge::AudioSound sound;
    };

    struct AudioConfig
    {
        std::vector<AudioFile> audioFiles;
    };
}
