#pragma once

#include <cstdint>
#include <string>

namespace ZeroX
{
	struct GameConfig
    {
        static constexpr const char* DEFAULT_GAME_LIBRARY_DIR       = "../../../../zerox_game/out/build/x64-Debug";
#ifdef _WIN32
        static constexpr const char* DEFAULT_GAME_LIBRARY_FILENAME  = "zerox_game.dll";
#elif __linux__
        static constexpr const char* DEFAULT_GAME_LIBRARY_FILENAME  = "libzerox_game.so";
#endif
        static constexpr unsigned int DEFAULT_MAX_QUEUED_FRAMES     = 1;

        std::string gameLibraryDir      { DEFAULT_GAME_LIBRARY_DIR };
        std::string gameLibraryFile     { DEFAULT_GAME_LIBRARY_FILENAME };
        unsigned int maxQueuedFrames    { DEFAULT_MAX_QUEUED_FRAMES };
    };
}
