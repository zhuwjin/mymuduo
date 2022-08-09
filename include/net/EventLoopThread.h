#ifndef MYMUDUO_EVENTLOOPTHREAD_H
#define MYMUDUO_EVENTLOOPTHREAD_H

#include "base/noncopyable.h"
#include <thread>
#include <string>
#include <functional>
#include <condition_variable>

class EventLoop;

class EventLoopThread : private noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    explicit EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), std::string name = std::string());

    ~EventLoopThread();

    EventLoop *startLoop();

private:
    std::string name_;
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};



#endif//MYMUDUO_EVENTLOOPTHREAD_H
