#ifndef MYMUDUO_TIMESTAMP_H
#define MYMUDUO_TIMESTAMP_H

#include "copyable.h"
#include <chrono>
#include <memory>
#include <string>

class Timestamp : public copyable {
public:
    friend bool operator<(const Timestamp &, const Timestamp &);
    friend bool operator==(const Timestamp &, const Timestamp &);

    using TimestampPtr = std::shared_ptr<Timestamp>;

    explicit Timestamp() : micro_seconds_since_epoch_(0) {}

    explicit Timestamp(time_t micro_seconds_since_epoch) : micro_seconds_since_epoch_(micro_seconds_since_epoch) {}

    static Timestamp now() {
        return Timestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }

    std::string toString() const {
        tm *ptm = localtime(&micro_seconds_since_epoch_);
        char str[128] = {0};
        sprintf(str, "%4d-%02d-%02d %02d:%02d:%02d",
                ptm->tm_year + 1900,
                ptm->tm_mon,
                ptm->tm_mday,
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
        return {str};
    }

    Timestamp operator+(double seconds) const {
        auto delta = static_cast<int64_t>(seconds * 1000);
        return Timestamp(micro_seconds_since_epoch_ + delta);
    }

    time_t microSecondsSinceEpoch() const {
        return micro_seconds_since_epoch_;
    }

private:
    time_t micro_seconds_since_epoch_;
};

inline bool operator<(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs.micro_seconds_since_epoch_ < rhs.micro_seconds_since_epoch_;
}

inline bool operator==(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs.micro_seconds_since_epoch_ == rhs.micro_seconds_since_epoch_;
}

#endif//MYMUDUO_TIMESTAMP_H
