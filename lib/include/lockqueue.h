#pragma once
#include <queue>              // 队列 std::queue
#include <thread>             // 线程 std::thread
#include <mutex>              // 互斥量 pthread_mutex_t
#include <condition_variable> // 条件变量 pthread_cond_t

// 异步写日志的队列
template <typename T>
class LockQueue
{
public:
    // 多个worker线程都会写日志queue，所以需要加锁
    void push(T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_cond.notify_one(); // 通知消费者
    }

    // 一个线程读日志queue，所以需要加锁
    T pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {
            m_cond.wait(lock); // 等待生产者
        }
        T data = m_queue.front();
        m_queue.pop();
        return data;
    }

private:
    std::queue<T> m_queue;          // 队列
    std::mutex m_mutex;             // 互斥量
    std::condition_variable m_cond; // 条件变量
};