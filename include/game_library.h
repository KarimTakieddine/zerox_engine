#pragma once

#include <memory>

#include <bump_allocator.hpp>
#include <platform.h>

#if defined(_WIN32) || defined(_WIN64)
    #define CDECL __cdecl
#else
    #define CDECL
#endif

namespace ZeroX
{
    constexpr size_t GAME_MEMORY_ALIGNMENT = 16;

    using GameAllocator = BumpAllocator<GAME_MEMORY_ALIGNMENT>;

    struct GameLibraryFunctions
    {
    public:
        typedef size_t(CDECL* GameInitFunc)(GameAllocator*);
        typedef bool(CDECL* GameUpdateFunc)(GameAllocator*, double);

        GameInitFunc initGame       { nullptr };
        GameUpdateFunc updateGame   { nullptr };
    };

    class GameLibrary
    {
    public:
        bool load(const char* path);

        bool isLoaded() const;
        bool isValid() const;

        const GameLibraryFunctions& getFunctions() const;

    private:
        GameLibraryFunctions m_functions;
        std::unique_ptr<platform::LibraryHandle> m_libraryHandle{ nullptr };
    };
}
