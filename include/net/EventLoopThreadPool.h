#ifndef MYMUDUO_EVENTLOOPTHREADPOOL_H
#define MYMUDUO_EVENTLOOPTHREADPOOL_H

#include "base/noncopyable.h"
#include <functional>
#include <string>
#include <thread>
#include <vector>

class EventLoop;

class EventLoopThread;

class EventLoopThreadPool : private noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *base_loop, std::string name);

    ~EventLoopThreadPool();

    void setThreadNum(int num) {
        num_threads_ = num;
    }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const {
        return started_;
    }

    const std::string &name() {
        return name_;
    }

private:
    EventLoop *base_loop_;
    std::string name_;
    bool started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};

#endif//MYMUDUO_EVENTLOOPTHREADPOOL_H
