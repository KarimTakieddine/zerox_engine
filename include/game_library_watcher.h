#pragma once

#include <thread>

#include <platform.h>

namespace ZeroX
{
    class GameLibraryWatcher
    {
    public:
        ~GameLibraryWatcher();

        bool startWatching(const char* parentDirectory, const char* filename);
        void stopWatching();
        uint64_t getChangeCounter() const;

    private:
        void execute();

        std::thread m_monitoringThread;
        platform::FileWriteEvent m_fileWriteEvent;
        std::atomic<uint64_t> m_fileChangeCounter{ 0 };
        std::atomic<bool> m_shouldRun{ true };
    };
}
