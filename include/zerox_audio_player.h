#pragma once

#include <cstdint>
#include <string>

#include "config/zerox_audio_config.h"

namespace ZeroX
{
    class IAudioPlayer
    {
    public:
        virtual ~IAudioPlayer() = default;

        virtual bool initialize(const AudioConfig& config) = 0;
        virtual bool loadSound(const AudioFile& file) = 0;
        virtual void unloadSound(size_t index) = 0;
        virtual bool playSound(size_t index) = 0;
    };
}
