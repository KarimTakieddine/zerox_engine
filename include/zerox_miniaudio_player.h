#pragma once

#include <memory>
#include <vector>

#include <miniaudio.h>

#include "zerox_audio_player.h"

namespace ZeroX
{
    class MiniAudioSound
    {
    public:
        ~MiniAudioSound();

        bool initialize(const char* file, ZeroXBridge::SoundType type, ma_engine* engine, ma_sound_group* group);
        bool start();

    private:
        ma_sound m_sound;
    };

    class MiniAudioPlayer : public IAudioPlayer
    {
    public:
        ~MiniAudioPlayer() override;

        bool initialize(const AudioConfig& config) override;
        bool loadSound(const AudioFile& file) override;
        void unloadSound(size_t index) override;
        bool playSound(size_t index) override;

    private:
        std::vector<std::unique_ptr<MiniAudioSound>> m_sounds;
        ma_engine m_engine;
        ma_sound_group m_sfxGroup;
        ma_sound_group m_bgmGroup;
    };
}
