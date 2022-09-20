#ifndef MYMUDUO_TIMER_H
#define MYMUDUO_TIMER_H

#include "Callbacks.h"
#include "base/Timestamp.h"
#include "base/noncopyable.h"
#include <atomic>

class Timer : noncopyable {
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(++s_num_created_) {}

    void run() const {
        callback_();
    }

    Timestamp expiration() const {
        return expiration_;
    }

    bool repeat() const {
        return repeat_;
    }

    int64_t sequence() const {
        return sequence_;
    }

    void restart(Timestamp now) {
        if (repeat_) {
            expiration_ = now + interval_;
        } else {
            expiration_ = Timestamp();
        }
    }

    static int64_t numCreated() {
        return s_num_created_;
    }

private:
    const TimerCallback callback_;
    Timestamp expiration_;  //到期
    const double interval_; //间隔
    const bool repeat_;     //重复
    const int64_t sequence_;//序列
    inline static std::atomic_int64_t s_num_created_;
};

#endif//MYMUDUO_TIMER_H
