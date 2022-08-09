#ifndef MYMUDUO_EPOLLPOLLER_H
#define MYMUDUO_EPOLLPOLLER_H

#include "Poller.h"

struct epoll_event;

/* 用epoll实现的事件监听 */

class EPollPoller : public Poller {
public:
    using ptr = std::shared_ptr<EPollPoller>;

    explicit EPollPoller(EventLoop *loop);

    ~EPollPoller() override;

    /* poll中若有事件发生， 则把epoll_wait()返回的fd们对应的channel放入active_channels*/
    Timestamp poll(int timeout_ms, ChannelList *active_channels) override;

    /* 更新，新增channel，并放入监听 */
    void updateChannel(Channel *channel) override;

    void removeChannel(Channel *channel) override;

private:
    static const int InitEventListSize = 16;

    void fillActivateChannels(int num_events, ChannelList *activate_channels) const;

    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

private:
    int epoll_fd_;
    EventList events_;
};


#endif//MYMUDUO_EPOLLPOLLER_H
