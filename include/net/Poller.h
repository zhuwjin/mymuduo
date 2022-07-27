#ifndef MYMUDUO_POLLER_H
#define MYMUDUO_POLLER_H

#include <unordered_map>
#include <vector>
#include "base/noncopyable.h"
#include "base/Timestamp.h"

class Channel;

class EventLoop;

class Poller : private noncopyable {
public:
    using ChannelList = std::vector<Channel *>;

    explicit Poller(EventLoop *loop);

    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList *active_channels) = 0;

    virtual void updateChannel(Channel *channel) = 0;

    virtual void removeChannel(Channel *channel) = 0;

    bool hasChannel(Channel *channel) const;

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;
private:
    EventLoop *owner_loop_;
};

#endif//MYMUDUO_POLLER_H
