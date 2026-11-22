#pragma once

#include <cstdint>

#include <string>
#include <vector>

#include <zerox_audio.h>

namespace ZeroX
{
    struct AudioFile
    {
        ZeroXBridge::AudioSound sound;
        std::string path;
    };

    struct AudioConfig
    {
        std::vector<AudioFile> audioFiles;
    };
}
