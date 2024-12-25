#pragma once
#include <string>
#include "lockqueue.h"

enum LogLevel
{
    INFO,    // 普通信息
    ERROR,   // 错误信息
    WARNING, // 警告信息
    FATAL    // 严重错误
};

// Mprpc框架提供的日志系统
/**
 * @file logger.h
 * @brief Logger class definition for logging functionality.
 */

/**
 * @class Logger
 * @brief A singleton class for logging messages with different log levels.
 *
 * The Logger class provides a thread-safe logging mechanism with different log levels.
 * It ensures that only one instance of the Logger exists throughout the application.
 */
class Logger
{
public:
    /**
     * @brief Get the singleton instance of the Logger.
     *
     * @return Logger& Reference to the singleton Logger instance.
     */
    static Logger &GetInstance();

    /**
     * @brief Set the log level for the Logger.
     *
     * @param level The log level to be set.
     */
    void SetLogLevel(LogLevel level);

    /**
     * @brief Log a message.
     *
     * @param msg The message to be logged.
     */
    void Log(std::string msg);

private:
    int m_logLevel;                       ///< The current log level.
    LockQueue<std::string> m_lcklogQueue; ///< A thread-safe queue to store log messages.

    /**
     * @brief Private constructor to prevent instantiation.
     */
    Logger();

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    Logger(const Logger &) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent copying.
     */
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief Deleted move constructor to prevent moving. std::move() is not allowed.
     */
    Logger(Logger &&) = delete;

    /**
     * @brief Deleted move assignment operator to prevent moving.
     */
    Logger &operator=(Logger &&) = delete;
};

// 定义宏调用
#define LOG_INFO(logmsgformat, ...)                     \
    do                                                  \
    {                                                   \
        Logger &logger = Logger::GetInstance();         \
        logger.SetLogLevel(INFO);                       \
        char c[1024] = {0};                             \
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c);                                  \
    } while (0);

// 在这个宏定义中，__VA_ARGS__ 代表可变参数列表。##__VA_ARGS__ 的作用是将可变参数列表展开并替换到宏定义中。如果没有提供可变参数，##操作符会移除##前面的1个逗号
#define LOG_ERROR(logmsgformat, ...)                    \
    do                                                  \
    {                                                   \
        Logger &logger = Logger::GetInstance();         \
        logger.SetLogLevel(ERROR);                      \
        char c[1024] = {0};                             \
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c);                                  \
    } while (0);
