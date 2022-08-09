#include "net/EventLoopThreadPool.h"

#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include <utility>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *base_loop, std::string name)
    : base_loop_(base_loop), started_(false),
      name_(std::move(name)), num_threads_(0), next_(0) {
}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::start(const EventLoopThreadPool::ThreadInitCallback &cb) {
    started_ = true;
    if (num_threads_ == 0 && cb) {
        cb(base_loop_);
    } else {
        for (int i = 0; i < num_threads_; ++i) {
            char buf[this->name_.size() + 32];
            snprintf(buf, sizeof(buf), "%s %d", name_.c_str(), i);
            auto t = std::make_unique<EventLoopThread>(cb, std::move(std::string(buf)));
            auto loop = t->startLoop();
            threads_.emplace_back(std::move(t));
            loops_.emplace_back(loop);
        }
    }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
    EventLoop *loop = base_loop_;
    if (!loops_.empty()) {
        loop = loops_[next_++];
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return {1, base_loop_};
    } else {
        return loops_;
    }
}