#include "logger.h"
#include <iostream>
#include <time.h>

Logger::Logger()
{
    // 启动专门的写日志线程
    std::thread writeLogTask([&]()
                             {
        while (true)
        {
            // 获取当前日期时间
            time_t t = time(0);
            tm *now = localtime(&t); // 获取本地时间
            char file_name[128] = {0};
            sprintf(file_name, "%d-%d-%d-log.txt", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);

            FILE *file = fopen(file_name, "a+");
            if (file == nullptr)
            {
                std::cout << "open file: "<<file_name << "error!" << std::endl;
                exit(EXIT_FAILURE);
            }

            // 从lockqueue中取出一条日志
            std::string msg = m_lcklogQueue.pop();

            char time_buf[128] = {0};
            sprintf(time_buf, "%d-%d-%d %d:%d:%d => [%s]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, m_logLevel == INFO ? "INFO" : "ERROR");
            msg.insert(0, time_buf);
            msg.append("\n");
            if (fputs(msg.c_str(), file) == EOF)
            {
                perror( "write log to file error!" );
            }
            fclose(file);
        } });
    // 设置分离线程，即守护线程，主线程结束，守护线程也会结束
    writeLogTask.detach();
    m_logLevel = INFO;
}

Logger &Logger::GetInstance()
{
    static Logger instance;
    return instance;
}

void Logger::SetLogLevel(LogLevel level)
{
    m_logLevel = level;
}

// 写日志，把日志写入lockqueue缓冲区
void Logger::Log(std::string msg)
{
    m_lcklogQueue.push(msg);
    // if (m_logLevel >= INFO)
    // {
    //     std::
}