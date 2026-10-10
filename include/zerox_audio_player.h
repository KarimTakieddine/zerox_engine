#pragma once

#include <cstdint>
#include <string>

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
        const char* file{ nullptr };
        SoundType type  { SoundType::UNKNOWN };
    };

    struct AudioConfig
    {
        AudioSound* sounds  { nullptr };
        size_t soundCount   { 0 };
    };

    class IAudioPlayer
    {
    public:
        virtual ~IAudioPlayer() = default;

        virtual bool initialize(const AudioConfig& config) = 0;
        virtual bool playSound(const std::string& name) = 0;
    };
}
