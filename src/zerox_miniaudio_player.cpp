#include <cassert>

#include "zerox_engine_logger.h"
#include "zerox_miniaudio_player.h"

namespace ZeroX
{
    MiniAudioSound::~MiniAudioSound()
    {
        ma_sound_uninit(&m_sound);
    }

    bool MiniAudioSound::initialize(const char* file, SoundType type, ma_engine* engine, ma_sound_group* group)
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
        case SoundType::SOUND_EFFECT:
            flags |= MA_SOUND_FLAG_DECODE;
            break;
        case SoundType::BGM:
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

        for (const auto& soundConfig : config.sounds)
        {
            auto sound = std::make_unique<MiniAudioSound>();

            bool result = false;

            switch (soundConfig.type)
            {
            case SoundType::BGM:
                result = sound->initialize(soundConfig.path.c_str(), soundConfig.type, &m_engine, &m_bgmGroup);
                break;
            case SoundType::SOUND_EFFECT:
                result = sound->initialize(soundConfig.path.c_str(), soundConfig.type, &m_engine, &m_sfxGroup);
                break;
            default:
                break;
            }
            
            if (!result)
            {
                return result;
            }

            m_sounds.emplace(std::make_pair(getFileBasename(soundConfig.path.c_str()), std::move(sound)));
        }

        return true;
    }
    
    bool MiniAudioPlayer::playSound(const std::string& name)
    {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end())
        {
            LOG_ERROR << "Failed to play audio sound. Could not find: " << name;
            return false;
        }

        return it->second->start();
    }
}