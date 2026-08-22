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

        m_functions.initGame();

        return true;
    }
}