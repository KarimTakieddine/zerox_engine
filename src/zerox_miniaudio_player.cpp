#include <cassert>

#include "config/zerox_audio_config.h"

#include "zerox_engine_logger.h"
#include "zerox_miniaudio_player.h"

namespace ZeroX
{
    MiniAudioSound::~MiniAudioSound()
    {
        ma_sound_uninit(&m_sound);
    }

    bool MiniAudioSound::initialize(const char* file, ZeroXBridge::SoundType type, ma_engine* engine, ma_sound_group* group)
    {
        assert(engine);
        if (!engine)
        {
            LOG_ERROR << "Failed to initialize sound from: " << file << " - No engine was supplied";
            return false;
        }

        ma_uint32 flags{ 0 };

        switch (type)
        {
        case ZeroXBridge::SoundType::SOUND_EFFECT:
            flags |= MA_SOUND_FLAG_DECODE;
            break;
        case ZeroXBridge::SoundType::BGM:
            flags |= MA_SOUND_FLAG_STREAM;
            break;
        default:
            break;
        }

        return ( ma_sound_init_from_file(engine, file, flags, group, nullptr, &m_sound) == MA_SUCCESS );
    }

    bool MiniAudioSound::start()
    {
        return ( ma_sound_start(&m_sound) == MA_SUCCESS );
    }

    MiniAudioPlayer::~MiniAudioPlayer()
    {
        m_sounds.clear();

        ma_engine_uninit(&m_engine);
    };

    bool MiniAudioPlayer::initialize(const AudioConfig& config)
    {
        if (ma_engine_init(nullptr, &m_engine) != MA_SUCCESS)
        {
            LOG_ERROR << "Failed to initialize autdio player: could not create miniaudio engine";
            return false;
        }

        if (ma_sound_group_init(&m_engine, 0, nullptr, &m_sfxGroup) != MA_SUCCESS)
        {
            LOG_ERROR << "Failed to initialize autdio player: could not create sfx sound group";
            return false;
        }

        if (ma_sound_group_init(&m_engine, 0, nullptr, &m_bgmGroup) != MA_SUCCESS)
        {
            LOG_ERROR << "Failed to initialize autdio player: could not create bgm sound group";
            return false;
        }

        bool allLoaded{ true };
        for (const auto& audioFile : config.audioFiles)
        {
            if (!loadSound(audioFile))
            {
                allLoaded = false;
            }
        }

        return allLoaded;
    }

    bool MiniAudioPlayer::loadSound(const AudioFile& file)
    {
        auto miniAudioSound = std::make_unique<MiniAudioSound>();

        bool result = false;

        const ZeroXBridge::AudioSound& sound = file.sound;

        switch (sound.type)
        {
        case ZeroXBridge::SoundType::BGM:
            result = miniAudioSound->initialize(file.path.c_str(), sound.type, &m_engine, &m_bgmGroup);
            break;
        case ZeroXBridge::SoundType::SOUND_EFFECT:
            result = miniAudioSound->initialize(file.path.c_str(), sound.type, &m_engine, &m_sfxGroup);
            break;
        default:
            break;
        }

        m_sounds.push_back(std::move(miniAudioSound));
            
        return result;
    }

    void MiniAudioPlayer::unloadSound(size_t index)
    {
        if (index >= m_sounds.size())
        {
            LOG_ERROR << "Failed to unload audio sound. Out-of-bounds index: " << std::to_string(index);
            return;
        }

        m_sounds.erase(m_sounds.begin() + index);
    }

    bool MiniAudioPlayer::playSound(size_t index)
    {
        if (index >= m_sounds.size())
        {
            LOG_ERROR << "Failed to play audio sound. Out-of-bounds index: " << std::to_string(index);
            return false;
        }

        return m_sounds.at(index)->start();
    }
}