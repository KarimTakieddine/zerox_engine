#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <bump_allocator.hpp>
#include <platform.h>

#include <zerox_game_context.h>
#include <zerox_render_descriptor.h>

#if defined(_WIN32) || defined(_WIN64)
    #define ZEROX_CDECL __cdecl
#else
    #define ZEROX_CDECL
#endif

namespace ZeroXBridge { struct EngineFunctions; }

namespace ZeroX
{
    using ZeroXBridge::RenderDescriptor;

    constexpr size_t GAME_MEMORY_ALIGNMENT = 16;

    using GameAllocator = BumpAllocator<GAME_MEMORY_ALIGNMENT>;

    struct GameLibraryFunctions
    {
    public:
        typedef void(ZEROX_CDECL* SetEngineFunctionsProc)(const ZeroXBridge::EngineFunctions*);
        typedef ZeroXBridge::GameContext(ZEROX_CDECL* GameInitFunc)(GameAllocator*, const ZeroXBridge::EngineFunctions*);
        typedef bool(ZEROX_CDECL* GameLoadLevelFunc)(const ZeroXBridge::GameContext*, GameAllocator*, size_t);
        typedef bool(ZEROX_CDECL* GameUpdateFunc)(GameAllocator*, double);
        typedef RenderDescriptor(ZEROX_CDECL* RenderDescriptorFunc)(const GameAllocator*);

        SetEngineFunctionsProc setEngineFunctions   { nullptr };
        GameInitFunc initGame                       { nullptr };
        GameLoadLevelFunc loadLevel                 { nullptr };
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

        std::string getWorkingDirectory() const;
        void setWorkingDirectory(const std::string& workingDirectory);

    private:
        mutable std::mutex m_workingDirectoryMutex;
        std::string m_workingDirectory;
        GameLibraryFunctions m_functions;
        platform::LibraryHandle m_handle{ nullptr };
    };
}
