#pragma once

#include <memory>

class Logger
{
public:
    enum class Level
    {
        ERROR,
        INFO,
        DEBUG,
    };

    explicit Logger(Logger::Level level);
    virtual ~Logger() = default;

    bool log(const Logger::Level level, const char *msg);

private:
    Level m_level;

    virtual void logMsg(const Logger::Level level, const char *msg) = 0;
};

class LoggerFactory
{
public:
    static std::unique_ptr<Logger> getFileLogger(const char *filename, Logger::Level level = Logger::Level::INFO);
    static std::unique_ptr<Logger> getConsoleLogger(Logger::Level level = Logger::Level::INFO);
};
