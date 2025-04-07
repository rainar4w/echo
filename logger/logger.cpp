#include <ctime>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#include <logger.h>

using namespace std;

namespace
{
    void logTime(ostringstream &oss)
    {
        time_t t = time(nullptr);
        if (((time_t)-1) == t)
        {
            cerr << "Failed to get current time" << endl;
            throw runtime_error("time");
        }

        // localtime is not thread-safe
        static mutex m;
        lock_guard<mutex> lock(m);

        tm *tm = localtime(&t);
        if (nullptr == localtime(&t))
        {
            cerr << "Failed to get local time" << endl;
            throw runtime_error("localtime");
        }

        oss << put_time(tm, "%F %T\t");
    }

    void logLevel(const Logger::Level level, ostringstream &oss)
    {
        static constexpr int width = 8;
        switch (level)
        {
        case Logger::Level::INFO:
            oss << left << setw(width) << "info";
            break;
        case Logger::Level::DEBUG:
            oss << left << setw(width) << "debug";
            break;
        case Logger::Level::ERROR:
            oss << left << setw(width) << "error";
            break;
        default:
            cerr << "Invalid log level " << static_cast<int>(level) << endl;
            throw runtime_error("invalid log level");
        }
    }

    class FileLogger : public Logger
    {
    public:
        explicit FileLogger(const char *filename, Logger::Level level)
            : Logger(level),
              m_ofs(filename, ios_base::out | ios_base::app)
        {
            if (!m_ofs.is_open())
            {
                cerr << "Failed to open log file " << filename << endl;
                ;
                throw runtime_error("Failed ot open log file");
            }
        }

        FileLogger(const FileLogger &l) = delete;
        const FileLogger &operator=(const FileLogger &l) = delete;

        ~FileLogger() override
        {
            m_ofs.close();
        }

    private:
        ofstream m_ofs;

        void logMsg([[maybe_unused]] const Logger::Level level, const char *msg) override
        {
            m_ofs << msg;
            m_ofs.flush();
        }
    };

    class ConsoleLogger : public Logger
    {
    public:
        explicit ConsoleLogger(Logger::Level level) : Logger(level)
        {
        }

    private:
        void logMsg(const Logger::Level level, const char *msg) override
        {
            switch (level)
            {
            case Level::INFO:
            case Level::DEBUG:
                cout << msg;
                break;
            case Level::ERROR:
                cerr << msg;
                break;
            default:
                cerr << "Invalid log level " << static_cast<int>(level) << endl;
                throw runtime_error("Invalid log level");
            }
        }
    };

} // namespace

Logger::Logger(Logger::Level level) : m_level(level)
{
}

bool Logger::log(const Logger::Level level, const char *msg)
{
    if (level > m_level)
        return true;

    try
    {
        ostringstream oss;
        logTime(oss);
        logLevel(level, oss);
        oss << msg << endl;
        logMsg(level, oss.str().c_str());
    }
    catch (const exception &e)
    {
        cerr << "Failed to log message " << msg;
        return false;
    }

    return true;
}

unique_ptr<Logger> LoggerFactory::getFileLogger(const char *filename, Logger::Level level /* = Logger::Level::INFO*/)
{
    return make_unique<FileLogger>(filename, level);
}

unique_ptr<Logger> LoggerFactory::getConsoleLogger(Logger::Level level /* = Logger::Level::INFO*/)
{
    return make_unique<ConsoleLogger>(level);
}
