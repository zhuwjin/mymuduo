#ifndef MYMUDUO_LOGGING_H
#define MYMUDUO_LOGGING_H

#include "Timestamp.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

/* 在析构函数中输出，可保证输出不乱序 */

class Logger {
public:
    enum LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };
    Logger(const char *source_file, int line, LogLevel level)
        : source_file_(source_file), line_(line), time_(Timestamp::now()), level_(level) {
        stream_ << time_.toString() << " " << std::this_thread::get_id() << " " << toString(level_) << " ";
    }

    template<typename T>
    Logger &operator<<(T &&t) {
        stream_ << std::forward<T>(t);
        return *this;
    }

    template<typename T>
    Logger &operator<<(const T &t) {
        stream_ << t;
        return *this;
    }


    ~Logger() {
        stream_ << " " << source_file_ << ":" << line_ << "\n";
        std::string buf = stream_.str();
        ::write(1, buf.c_str(), buf.size());
    }

    static LogLevel getGLevel() {
        return g_level_;
    }

    static void setGLevel(LogLevel level) {
        g_level_ = level;
    }

private:
    static const char *toString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE:
                return "TRACE";
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARN:
                return "WARN";
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::FATAL:
                return "FATAL";
        }
    }
    std::string source_file_;
    int line_;
    LogLevel level_;
    Timestamp time_;
    std::stringstream stream_;
    inline static LogLevel g_level_ = INFO;
};

#define LOG_INFO \
    if (Logger::getGLevel() <= Logger::LogLevel::INFO) Logger(__FILE__, __LINE__, Logger::INFO)
#define LOG_TRACE \
    if (Logger::getGLevel() <= Logger::LogLevel::TRACE) Logger(__FILE__, __LINE__, Logger::TRACE)
#define LOG_DEBUG \
    if (Logger::getGLevel() <= Logger::LogLevel::DEBUG) Logger(__FILE__, __LINE__, Logger::DEBUG)
#define LOG_WARN \
    if (Logger::getGLevel() <= Logger::LogLevel::WARN) Logger(__FILE__, __LINE__, Logger::WARN)
#define LOG_ERROR \
    if (Logger::getGLevel() <= Logger::LogLevel::ERROR) Logger(__FILE__, __LINE__, Logger::ERROR)
#define LOG_FATAL \
    if (Logger::getGLevel() <= Logger::LogLevel::FATAL) Logger(__FILE__, __LINE__, Logger::FATAL)
#endif//MYMUDUO_LOGGING_H
