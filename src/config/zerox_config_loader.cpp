#include <stdexcept>

#include "config/zerox_config_loader.h"
#include "config/zerox_config_parser.h"

namespace ZeroX
{
	std::future<WindowConfig> loadWindowConfig(const pugi::xml_node& node)
	{
		return std::async(
			std::launch::async, [node]() -> WindowConfig {

				WindowConfig result = { };
				if (node)
				{
					result = parseWindowConfig(node);

					setReasonableDefaults(result);
				}

				return result;
			}
		);
	}

	std::future<AudioConfig> loadAudioConfig(const pugi::xml_node& node)
	{
		return std::async(
			std::launch::async, [node]() -> AudioConfig {

				AudioConfig result = { };
				if (node)
				{
					result = parseAudioConfig(node);

					// TODO(Karim): Implement: setReasonableDefaults(result);
				}

				return result;
			}
		);
	}

	std::future<GameConfig> loadGameConfig(const pugi::xml_node& node)
	{
		return std::async(
			std::launch::async, [node]() -> GameConfig {

				GameConfig result = { };
				if (node)
				{
					result = parseGameConfig(node);

					setReasonableDefaults(result);
				}

				return result;
			}
		);
	}
}