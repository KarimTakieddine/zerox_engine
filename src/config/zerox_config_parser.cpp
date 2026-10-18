#include <pugixml.hpp>

#include "config/zerox_config_parser.h"

namespace
{
	bool parseWindowConfig(const pugi::xml_node& node, ZeroX::WindowConfig& config, ZeroX::ParseError* error)
	{
		if (pugi::xml_attribute widthAttribute = node.attribute("width"))							{ config.width = widthAttribute.as_uint(); }
		if (pugi::xml_attribute heightAttribute = node.attribute("height"))							{ config.height = heightAttribute.as_uint(); }
		if (pugi::xml_attribute nameAttribute = node.attribute("name"))								{ config.name = nameAttribute.as_string(); }
		if (pugi::xml_attribute openGLVersionMajorAttribute = node.attribute("openGLVersionMajor")) { config.openGLVersionMajor = openGLVersionMajorAttribute.as_uint(); }
		if (pugi::xml_attribute openGLVersionMinorAttribute = node.attribute("openGLVersionMinor")) { config.openGLVersionMinor = openGLVersionMinorAttribute.as_uint(); }
		if (pugi::xml_attribute resizeableAttribute = node.attribute("resizeable"))					{ config.resizeable = resizeableAttribute.as_bool(); }
		if (pugi::xml_attribute fullscreenAttribute = node.attribute("fullscreen"))					{ config.fullscreen = fullscreenAttribute.as_bool(); }

		return true;
	}

	bool parseGameConfig(const pugi::xml_node& node, ZeroX::GameConfig& config, ZeroX::ParseError* error)
	{
		if (pugi::xml_attribute libraryDirAttribute	= node.attribute("libraryDir"))				{ config.gameLibraryDir = libraryDirAttribute.as_string(); }
		if (pugi::xml_attribute libraryFileAttribute = node.attribute("libraryFile"))			{ config.gameLibraryFile = libraryFileAttribute.as_string(); }
		if (pugi::xml_attribute maxQueuedFramesAttribute = node.attribute("maxQueuedFrames"))	{ config.maxQueuedFrames = maxQueuedFramesAttribute.as_uint(); }

		return true;
	}
}

namespace ZeroX
{
	bool parseEngineConfig(const char* file, EngineConfig& config, ParseError* error)
	{
		pugi::xml_document document;
		pugi::xml_parse_result result = document.load_file(file, pugi::parse_minimal, pugi::xml_encoding::encoding_utf8);
		if (!result)
		{
			if (error)
			{
				error->description	= result.description();
				error->code			= ParseErrorCode::LOAD_ERROR;
			}

			return false;
		}

		pugi::xml_node engineNode = document.root().first_child();
		if (!engineNode)
		{
			if (error)
			{
				error->description	= result.description();
				error->code			= ParseErrorCode::ENGINE_CONFIG_ERROR;

				return false;
			}
		}

		pugi::xml_node windowConfigNode = engineNode.child("Window");
		if (!windowConfigNode)
		{
			if (error)
			{
				error->description = "Could not find <Window/> node";
				error->code			= ParseErrorCode::WINDOW_CONFIG_ERROR;

				return false;
			}
		}

		if (!::parseWindowConfig(windowConfigNode, config.windowConfig, error))
		{
			return false;
		}

		pugi::xml_node gameConfigNode = engineNode.child("Game");
		if (!gameConfigNode)
		{
			if (error)
			{
				error->description = "Could not find <Game/> node";
				error->code			= ParseErrorCode::GAME_CONFIG_ERROR;

				return false;
			}
		}

		if (!::parseGameConfig(gameConfigNode, config.gameConfig, error))
		{
			return false;
		}

		return true;
	}

	void setReasonableDefaults(EngineConfig& config)
	{
		WindowConfig& windowConfig = config.windowConfig;

		if ( ( windowConfig.width == 0 ) || ( windowConfig.width > 3840 ) )
		{
			windowConfig.width = WindowConfig::DEFAULT_WIDTH;
		}

		if ( ( windowConfig.height == 0 ) || ( windowConfig.height > 2160 ) )
		{
			windowConfig.height = WindowConfig::DEFAULT_HEIGHT;
		}

		if ( ( windowConfig.openGLVersionMajor < 1 ) || ( windowConfig.openGLVersionMajor > WindowConfig::DEFAULT_OPENGL_VERSION_MAJOR ) )
		{
			windowConfig.openGLVersionMajor = WindowConfig::DEFAULT_OPENGL_VERSION_MAJOR;
		}

		GameConfig& gameConfig = config.gameConfig;

		if (gameConfig.gameLibraryDir.empty())
		{
			gameConfig.gameLibraryDir = GameConfig::DEFAULT_GAME_LIBRARY_DIR;
		}

		if (gameConfig.gameLibraryFile.empty())
		{
			gameConfig.gameLibraryFile = GameConfig::DEFAULT_GAME_LIBRARY_FILENAME;
		}

		if (gameConfig.maxQueuedFrames < 1)
		{
			gameConfig.maxQueuedFrames = GameConfig::DEFAULT_MAX_QUEUED_FRAMES;
		}
	}
}