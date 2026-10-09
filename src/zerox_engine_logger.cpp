#include <iostream>
#include <utility>

#include "zerox_engine_logger.h"

namespace ZeroX
{
    EngineLogger::~EngineLogger()
    {
        terminate();
    }

    void EngineLogger::initialize()
    {
        m_executionThread = std::thread(&EngineLogger::execute, this);
    }

    void EngineLogger::enqueueMessage(const std::string& message)
    {
        m_transientQueue.push_back(message);
    }

    void EngineLogger::processMessages()
    {
        std::unique_lock<std::mutex> lock(m_messageMutex);
        std::swap(m_transientQueue, m_messageQueue);
        lock.unlock();

        m_processSemaphore.notify_one();
    }

    void EngineLogger::terminate()
    {
        m_shouldRun.store(false, std::memory_order_release);

        m_processSemaphore.notify_one();

        if (m_executionThread.joinable())
        {
            m_executionThread.join();
        }
    }

    void EngineLogger::execute()
    {
        while (m_shouldRun.load(std::memory_order_acquire))
        {
            std::unique_lock<std::mutex> lock(m_messageMutex);
    
            m_processSemaphore.wait(lock, [&]() -> bool { return !m_shouldRun.load(std::memory_order_acquire) || !m_messageQueue.empty(); });

            std::deque<std::string> threadLocalQueue;
            std::swap(m_messageQueue, threadLocalQueue);
            lock.unlock();

            while (!threadLocalQueue.empty())
            {
                std::cout << threadLocalQueue.front() << std::endl;
                threadLocalQueue.pop_front();
            }
        }
    }
}