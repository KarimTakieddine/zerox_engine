#include <unordered_map>

#include <pugixml.hpp>

#include "config/zerox_config_parser.h"

namespace
{
	const std::unordered_map<std::string, ZeroX::SoundType> STRING_TO_SOUND_TYPE = {
		{ "effect", ZeroX::SoundType::SOUND_EFFECT },
		{ "music", ZeroX::SoundType::BGM }
	};

	ZeroX::SoundType parseSoundType(const std::string& type)
	{
		const auto it = STRING_TO_SOUND_TYPE.find(type);
		if (it == STRING_TO_SOUND_TYPE.cend())
		{
			return ZeroX::SoundType::UNKNOWN;
		}

		return it->second;
	}

	ZeroX::AudioSound parseAudioSound(const pugi::xml_node node)
	{
		ZeroX::AudioSound sound;

		if (pugi::xml_attribute pathAttribute = node.attribute("path")) { sound.path = pathAttribute.as_string(); }
		if (pugi::xml_attribute typeAttribute = node.attribute("type")) { sound.type = parseSoundType(typeAttribute.as_string()); }

		return sound;
	}
}

namespace ZeroX
{
	WindowConfig parseWindowConfig(const pugi::xml_node& node)
	{
		WindowConfig config;

		if (pugi::xml_attribute widthAttribute = node.attribute("width"))							{ config.width = widthAttribute.as_uint(); }
		if (pugi::xml_attribute heightAttribute = node.attribute("height"))							{ config.height = heightAttribute.as_uint(); }
		if (pugi::xml_attribute nameAttribute = node.attribute("name"))								{ config.name = nameAttribute.as_string(); }
		if (pugi::xml_attribute openGLVersionMajorAttribute = node.attribute("openGLVersionMajor")) { config.openGLVersionMajor = openGLVersionMajorAttribute.as_uint(); }
		if (pugi::xml_attribute openGLVersionMinorAttribute = node.attribute("openGLVersionMinor")) { config.openGLVersionMinor = openGLVersionMinorAttribute.as_uint(); }
		if (pugi::xml_attribute resizeableAttribute = node.attribute("resizeable"))					{ config.resizeable = resizeableAttribute.as_bool(); }
		if (pugi::xml_attribute fullscreenAttribute = node.attribute("fullscreen"))					{ config.fullscreen = fullscreenAttribute.as_bool(); }

		return config;
	}

	GameConfig parseGameConfig(const pugi::xml_node& node)
	{
		GameConfig config;

		if (pugi::xml_attribute libraryDirAttribute	= node.attribute("libraryDir"))				{ config.gameLibraryDir = libraryDirAttribute.as_string(); }
		if (pugi::xml_attribute libraryFileAttribute = node.attribute("libraryFile"))			{ config.gameLibraryFile = libraryFileAttribute.as_string(); }
		if (pugi::xml_attribute maxQueuedFramesAttribute = node.attribute("maxQueuedFrames"))	{ config.maxQueuedFrames = maxQueuedFramesAttribute.as_uint(); }

		return config;
	}

	AudioConfig parseAudioConfig(const pugi::xml_node& node)
	{
		AudioConfig config;

		for (pugi::xml_node soundNode: node.children("Sound"))
		{
			config.sounds.push_back(::parseAudioSound(soundNode));
		}

		return config;
	}

	void setReasonableDefaults(WindowConfig& config)
	{
		if ( (config.width == 0 ) || (config.width > 3840 ) )
		{
			config.width = WindowConfig::DEFAULT_WIDTH;
		}

		if ( (config.height == 0 ) || (config.height > 2160 ) )
		{
			config.height = WindowConfig::DEFAULT_HEIGHT;
		}

		if ( (config.openGLVersionMajor < 1 ) || (config.openGLVersionMajor > WindowConfig::DEFAULT_OPENGL_VERSION_MAJOR ) )
		{
			config.openGLVersionMajor = WindowConfig::DEFAULT_OPENGL_VERSION_MAJOR;
		}
	}

	void setReasonableDefaults(GameConfig& config)
	{
		if (config.gameLibraryDir.empty())
		{
			config.gameLibraryDir = GameConfig::DEFAULT_GAME_LIBRARY_DIR;
		}

		if (config.gameLibraryFile.empty())
		{
			config.gameLibraryFile = GameConfig::DEFAULT_GAME_LIBRARY_FILENAME;
		}

		if (config.maxQueuedFrames < 1)
		{
			config.maxQueuedFrames = GameConfig::DEFAULT_MAX_QUEUED_FRAMES;
		}
	}
}