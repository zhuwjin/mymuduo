#include "net/EventLoopThread.h"

#include <utility>
#include "net/EventLoop.h"

EventLoopThread::EventLoopThread(ThreadInitCallback cb, std::string name)
    : loop_(nullptr), exiting_(false),
      name_(std::move(name)),
      thread_([this] { threadFunc(); }),
      mutex_(), cond_(), callback_(std::move(cb)) {}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop() {
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (this->loop_ == nullptr) {
            cond_.wait(lock);
        }
        loop = this->loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;
    if (callback_) {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}