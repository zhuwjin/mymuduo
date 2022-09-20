#ifndef MYMUDUO_TIMERQUEUE_H
#define MYMUDUO_TIMERQUEUE_H
#include "Callbacks.h"
#include "Channel.h"
#include "base/Timestamp.h"
#include "base/noncopyable.h"
#include <set>
#include <utility>
#include <vector>

class EventLoop;

class Timer;

class TimerId;

class TimerQueue : noncopyable {
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

private:
    using Entry = std::pair<Timestamp, Timer *>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer *, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;
    void addTimerInLoop(Timer *timer);
    void cancelInLoop(TimerId timer_id);
    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry> &expired, Timestamp now);
    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel channel_;
    TimerList timers_;
    ActiveTimerSet active_timers_;
    bool calling_expired_timers_;
    ActiveTimerSet canceling_timers_;
};

#endif//MYMUDUO_TIMERQUEUE_H
