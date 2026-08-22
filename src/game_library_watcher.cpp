#include "game_library_watcher.h"

namespace ZeroX
{
    GameLibraryWatcher::~GameLibraryWatcher()
    {
        if (m_monitoringThread.joinable())
        {
            m_monitoringThread.join();
        }
    }

    bool GameLibraryWatcher::startWatching(const char* path)
    {
        if (!platform::initFileWriteEvent(path, &m_fileWriteEvent))
        {
            return false;
        }

        m_monitoringThread = std::thread(&GameLibraryWatcher::execute, this);

        return true;
    }

    void GameLibraryWatcher::stopWatching()
    {
        m_shouldRun.store(false, std::memory_order_release);

        platform::stopFileWatch(&m_fileWriteEvent);

        if (m_monitoringThread.joinable())
        {
            m_monitoringThread.join();
        }
    }

    uint64_t GameLibraryWatcher::getChangeCounter() const
    {
        return m_fileChangeCounter.load(std::memory_order_acquire);
    }

    void GameLibraryWatcher::execute()
    {
        while (m_shouldRun.load(std::memory_order_acquire))
        {
            if (!platform::watchFileWrites(&m_fileWriteEvent, &m_fileChangeCounter))
            {
                break;
            }
        }

        platform::closeFileWriteEvent(&m_fileWriteEvent);
    }
}