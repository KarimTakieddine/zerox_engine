#include <stdexcept>

#include "config/zerox_config_loader.h"
#include "config/zerox_config_parser.h"

namespace ZeroX
{
	ConfigLoader::~ConfigLoader()
	{
		if (m_loadThread.joinable())
		{
			m_loadThread.join();
		}
	}

	std::future<GameConfig> ConfigLoader::getGameConfigFuture()
	{
		return m_gameConfigPromise.get_future();
	}

	std::future<WindowConfig> ConfigLoader::getWindowConfigFuture()
	{
		return m_windowConfigPromise.get_future();
	}

	void ConfigLoader::load(const char* file)
	{
		m_loadThread = std::thread(&ConfigLoader::execute, this, file);
	}

	void ConfigLoader::execute(const char* file)
	{
		pugi::xml_document document;
		pugi::xml_parse_result parse_result = document.load_file(file, pugi::parse_minimal, pugi::xml_encoding::encoding_utf8);
		if (!parse_result)
		{
			auto e = std::make_exception_ptr(std::runtime_error(parse_result.description()));

			m_windowConfigPromise.set_exception(e);
			m_gameConfigPromise.set_exception(e);

			return;
		}

		pugi::xml_node root = document.root().first_child();
		if (!root)
		{
			auto e = std::make_exception_ptr(std::runtime_error("No document root element was found"));

			m_windowConfigPromise.set_exception(e);
			m_gameConfigPromise.set_exception(e);

			return;
		}

		pugi::xml_node windowConfigNode = root.child("Window");
		WindowConfig windowConfig		= { };

		if (windowConfigNode)
		{
			windowConfig = parseWindowConfig(windowConfigNode);

			setReasonableDefaults(windowConfig);
		}

		m_windowConfigPromise.set_value(windowConfig);

		pugi::xml_node gameConfigNode	= root.child("Game");
		GameConfig gameConfig			= { };

		if (gameConfigNode)
		{
			gameConfig = parseGameConfig(gameConfigNode);

			setReasonableDefaults(gameConfig);
		}

		m_gameConfigPromise.set_value(gameConfig);
	}
}