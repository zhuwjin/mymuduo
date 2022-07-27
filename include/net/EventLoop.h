#ifndef MYMUDUO_EVENTLOOP_H
#define MYMUDUO_EVENTLOOP_H

#include "Callbacks.h"
#include "base/Timestamp.h"
#include "base/noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

class Channel;

class TimerId;

class TimerQueue;

class Poller;

/* 事件循环
 * 在循环中执行Poller::poll获得发生事件的channel
 * 在执行channel::handleEvent执行事件对应的回调
 * */

class EventLoop : private noncopyable {
public:
    using ptr = std::shared_ptr<EventLoop>;
    using Functor = std::function<void()>;
    EventLoop();

    ~EventLoop();

    void loop();

    void quit();

    Timestamp pollReturnTime() const {
        return poll_return_time_;
    }

    TimerId runAt(const Timestamp& time, const TimerCallback& cb);

    TimerId runAfter(double delay, const TimerCallback& cb);

    TimerId runEvery(double interval, const TimerCallback &cb);

    void runInLoop(Functor cb);

    void queueInLoop(Functor cb);

    void wakeup();

    void updateChannel(Channel *channel);

    void removeChannel(Channel *channel);

    bool hasChannel(Channel *channel);

    bool isInLoopThread() const;

private:
    using ChannelList = std::vector<Channel *>;

    void handleRead();

    void doPendingFunctors();

    std::atomic_bool looping_;
    std::atomic_bool quit_;
    const std::thread::id thread_id_;
    int wakeup_fd_;
    Timestamp poll_return_time_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timer_queue_;
    std::unique_ptr<Channel> wakeup_channel_;
    ChannelList active_channels_;
    std::atomic_bool calling_pending_functions_;
    std::vector<Functor> pending_functors_;
    std::mutex mutex_;
};

#endif//MYMUDUO_EVENTLOOP_H
