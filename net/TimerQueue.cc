#include "net/TimerQueue.h"
#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/Timer.h"
#include "net/TimerId.h"

#include <sys/timerfd.h>
#include <unistd.h>

int createTimerfd() {//创建一个timerfd
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        LOG_FATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {//返回给定Timestamp距离现在还有多长时间
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;
    }
    struct timespec ts {};
    ts.tv_sec = static_cast<time_t>(microseconds / 1000);
    ts.tv_nsec = static_cast<long>(microseconds % 1000 * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany) {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

void resetTimerfd(int timerfd, Timestamp expiration) {//将timerfd的超时时间设置到expiration
    struct itimerspec new_value {};
    struct itimerspec old_value {};
    new_value.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
    if (ret) {
        LOG_ERROR << "timerfd_settime()";
    }
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop), timerfd_(createTimerfd()),
      channel_(loop, timerfd_),
      timers_(), calling_expired_timers_(false) {
    channel_.setReadCallback([this](Timestamp) { handleRead(); });//有超时事件发生
    channel_.enableReading();
}

TimerQueue::~TimerQueue() {
    channel_.disableAll();
    channel_.remove();
    ::close(timerfd_);
    for (const Entry &timer: timers_) {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    auto *timer = new Timer(std::move(cb), when, interval);//创建一个Timer
    loop_->runInLoop([this, timer] { addTimerInLoop(timer); });
    return {timer, timer->sequence()};
}

void TimerQueue::addTimerInLoop(Timer *timer) {
    bool earliest_changed = insert(timer);//插入
    if (earliest_changed) {//如果插入的Timer的到期时间小于已存在的Timer的最小到期时间
        resetTimerfd(timerfd_, timer->expiration());//设置timerfd的到期时间为该timer
    }
}

void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop([this, timerId] { cancelInLoop(timerId); });
}

void TimerQueue::cancelInLoop(TimerId timer_id) {
    ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
    auto it = active_timers_.find(timer);
    if (it != active_timers_.end()) {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        delete it->first;
        active_timers_.erase(it);
    } else if (calling_expired_timers_) {
        canceling_timers_.insert(timer);
    }
}

void TimerQueue::handleRead() {//超时事件发送，执行回调
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);//获取已到期的Timer
    calling_expired_timers_ = true;
    canceling_timers_.clear();
    for (const Entry &it: expired) {
        it.second->run();//执行已超时的Timer的回调
    }
    calling_expired_timers_ = false;
    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {//返回已到期的Timer
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    auto end = timers_.lower_bound(sentry);                 //找到第一个 > now 的值
    std::copy(timers_.begin(), end, back_inserter(expired));//将小于等于now的放入expired中
    timers_.erase(timers_.begin(), end);                    //删除

    for (const Entry &it: expired) {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = active_timers_.erase(timer);//从active_timers中删除
    }
    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
    Timestamp next_expire;
    for (const Entry &it: expired) {//遍历已到期的Timers
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat() && canceling_timers_.find(timer) == canceling_timers_.end()) {//如果timer是重复的，且不在canceling中
            it.second->restart(now);                                                          //重置timer
            insert(it.second);                                                                //插入
        } else {
            delete it.second;//否则删除
        }
    }
    if (!timers_.empty()) {
        next_expire = timers_.begin()->second->expiration();
    }

    if (next_expire.microSecondsSinceEpoch() > 0) {
        resetTimerfd(timerfd_, next_expire);//设置到期时间为下一个Timer
    }
}

bool TimerQueue::insert(Timer *timer) {
    bool earliest_changed = false;
    Timestamp when = timer->expiration();
    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
        earliest_changed = true;
    }
    timers_.insert(Entry(when, timer));//插入timer
    active_timers_.insert(ActiveTimer(timer, timer->sequence()));
    return earliest_changed;
}
