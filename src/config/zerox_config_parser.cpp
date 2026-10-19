#include <pugixml.hpp>

#include "config/zerox_config_parser.h"

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