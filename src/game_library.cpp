#include "game_library.h"

namespace ZeroX
{
    GameLibrary::~GameLibrary()
    {
        platform::closeLibrary(m_handle);
    }

    bool GameLibrary::load(const char* path)
    {
        m_handle = platform::openLibrary(path);

        if (!m_handle)
        {
            return false;
        }

        m_functions.initGame    = (GameLibraryFunctions::GameInitFunc)platform::loadLibraryFunction(m_handle, "initializeGame");
        m_functions.updateGame  = (GameLibraryFunctions::GameUpdateFunc)platform::loadLibraryFunction(m_handle, "updateGame");

        return true;
    }

    bool GameLibrary::isLoaded() const
    {
        return m_handle;
    }

    bool GameLibrary::isValid() const
    {
        return m_functions.initGame && m_functions.updateGame;
    }

    const GameLibraryFunctions& GameLibrary::getFunctions() const
    {
        return m_functions;
    }
}