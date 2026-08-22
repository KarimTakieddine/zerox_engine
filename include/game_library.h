#pragma once

#include <memory>

#include <platform.h>

#if defined(_WIN32) || defined(_WIN64)
    #define CDECL __cdecl
#else
    #define CDECL
#endif

namespace ZeroX
{
    struct GameLibraryFunctions
    {
    public:
        typedef size_t(CDECL* GameInitFunc)();
        typedef bool(CDECL* GameUpdateFunc)();

        GameInitFunc initGame       { nullptr };
        GameUpdateFunc updateGame   { nullptr };
    };

    class GameLibrary
    {
    public:
        bool load(const char* path);

    private:
        GameLibraryFunctions m_functions;
        std::unique_ptr<platform::LibraryHandle> m_libraryHandle{ nullptr };
    };
}
