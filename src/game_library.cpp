#include "game_library.h"

namespace ZeroX
{
    bool GameLibrary::load(const char* path)
    {
        m_libraryHandle = platform::openLibrary(path);

        if (!m_libraryHandle || !m_libraryHandle->getHandle())
        {
            return false;
        }

        m_functions.initGame    = (GameLibraryFunctions::GameInitFunc)platform::loadLibraryFunction(m_libraryHandle.get(), "initializeGame");
        m_functions.updateGame  = (GameLibraryFunctions::GameUpdateFunc)platform::loadLibraryFunction(m_libraryHandle.get(), "updateGame");

        return true;
    }

    bool GameLibrary::isLoaded() const
    {
        return m_libraryHandle && m_libraryHandle->getHandle();
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