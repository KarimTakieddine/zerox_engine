#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <platform.h>

namespace ZeroX
{
    inline std::string getFileBasename(const char* file)
    {
        const std::string path(file);
        if (path.empty())
        {
            return "NON_FILE";
        }

        auto pos = path.rfind('/', path.size() - 1);
        if (pos == std::string::npos)
        {
            pos = path.rfind('\\', path.size() - 1);
        }

        if (pos == std::string::npos)
        {
            return "NON_FILE";
        }

        return path.substr(pos + 1, path.size() - 1 - pos);
    }

    enum class LogLevel : uint8_t
    {
        LEVEL_UNKNOWN   = 0,
        LEVEL_INFO      = 1,
        LEVEL_ERROR     = 2,
        LEVEL_WARNING   = 3
    };

    inline std::stringstream& operator<<(std::stringstream& stream, LogLevel logLevel)
    {
        switch (logLevel)
        {
        case LogLevel::LEVEL_INFO:
            stream << "INFO";
            break;
        case LogLevel::LEVEL_WARNING:
            stream << "WARN";
            break;
        case LogLevel::LEVEL_ERROR:
            stream << "ERRO";
            break;
        default:
            stream << "UNKN";
            break;
        }

        return stream;
    }

    class EngineLogger
    {
    public:
        ~EngineLogger();

        void initialize();
        void enqueueMessage(const std::string& message);
        void processMessages();
        void terminate();

    private:
        void execute();
    
        std::thread m_executionThread;
        std::mutex m_messageMutex;
        std::deque<std::string> m_messageQueue;
        std::condition_variable m_processSemaphore;
        std::atomic<bool> m_shouldRun{ true };
    };

    inline EngineLogger* globalEngineLogger{ nullptr };

    class LogMessageBuilder
    {
    public:
        LogMessageBuilder(LogLevel level, const char* file, uint64_t line)
        {
            m_stream << platform::getFormattedDateTime() << " - ";
            m_stream << level << "[" << getFileBasename(file) << ':' << line << "] ";
        }

        template<typename T>
        LogMessageBuilder& operator<<(const T& message)
        {
            m_stream << message;

            return *this;
        }

        ~LogMessageBuilder()
        {
            globalEngineLogger->enqueueMessage(m_stream.str());
        }
    
    private:
        std::stringstream m_stream;
    };
}

#define LOG_INFO assert(ZeroX::globalEngineLogger); ZeroX::LogMessageBuilder(ZeroX::LogLevel::LEVEL_INFO, __FILE__, __LINE__)
#define LOG_ERROR assert(ZeroX::globalEngineLogger); ZeroX::LogMessageBuilder(ZeroX::LogLevel::LEVEL_ERROR, __FILE__, __LINE__)
#define LOG_WARN assert(ZeroX::globalEngineLogger); ZeroX::LogMessageBuilder(ZeroX::LogLevel::LEVEL_WARNING, __FILE__, __LINE__)
