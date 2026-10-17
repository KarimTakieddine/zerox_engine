#include "game_library.h"

namespace ZeroX
{
    GameLibrary::~GameLibrary()
    {
        unload();
    }

    bool GameLibrary::load(const char* path)
    {
        m_handle = platform::openLibrary(path);

        if (!m_handle)
        {
            return false;
        }

        auto a = platform::loadLibraryFunction(m_handle, "initializeRenderDescriptor");

        m_functions = {
            .initGame               = (GameLibraryFunctions::GameInitFunc)platform::loadLibraryFunction(m_handle, "initializeGame"),
            .updateGame             = (GameLibraryFunctions::GameUpdateFunc)platform::loadLibraryFunction(m_handle, "updateGame"),
            .initRenderDescriptor   = (GameLibraryFunctions::RenderDescriptorFunc)platform::loadLibraryFunction(m_handle, "initializeRenderDescriptor")
        };

        return isValid();
    }

    void GameLibrary::unload() const
    {
        platform::closeLibrary(m_handle);
    }

    bool GameLibrary::isLoaded() const
    {
        return m_handle;
    }

    bool GameLibrary::isValid() const
    {
        return
            m_functions.initGame &&
            m_functions.updateGame &&
            m_functions.initRenderDescriptor;
    }

    const GameLibraryFunctions& GameLibrary::getFunctions() const
    {
        return m_functions;
    }
}