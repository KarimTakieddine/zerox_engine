#pragma once

#include <memory>

#include <bump_allocator.hpp>
#include <platform.h>

#include "zerox_render_descriptor.h"

#if defined(_WIN32) || defined(_WIN64)
    #define ZEROX_CDECL __cdecl
#else
    #define ZEROX_CDECL
#endif

namespace ZeroX
{
    constexpr size_t GAME_MEMORY_ALIGNMENT = 16;

    using GameAllocator = BumpAllocator<GAME_MEMORY_ALIGNMENT>;

    struct GameLibraryFunctions
    {
    public:
        typedef bool(ZEROX_CDECL* GameInitFunc)(GameAllocator*);
        typedef bool(ZEROX_CDECL* GameUpdateFunc)(GameAllocator*, double);
        typedef RenderDescriptor(ZEROX_CDECL* RenderDescriptorFunc)(const GameAllocator*);

        GameInitFunc initGame                       { nullptr };
        GameUpdateFunc updateGame                   { nullptr };
        RenderDescriptorFunc initRenderDescriptor   { nullptr };
    };

    class GameLibrary
    {
    public:
        ~GameLibrary();

        bool load(const char* path);
        void unload() const;

        bool isLoaded() const;
        bool isValid() const;

        const GameLibraryFunctions& getFunctions() const;

    private:
        GameLibraryFunctions m_functions;
        platform::LibraryHandle m_handle{ nullptr };
    };
}
