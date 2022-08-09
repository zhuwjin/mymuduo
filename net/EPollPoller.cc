#include "net/EPollPoller.h"
#include "net/Channel.h"
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>


EPollPoller::EPollPoller(EventLoop *loop) : Poller(loop),
                                            epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
                                            events_(InitEventListSize) {
    if (epoll_fd_ < 0) {
        LOG_FATAL << "EPollPoller::EpollPoller error";
    }
}

EPollPoller::~EPollPoller() noexcept {
    close(this->epoll_fd_);
}

void EPollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);//从channels中移除
    LOG_TRACE << "remove channel fd:" << fd;
    if (channel->index() == 1) {//如果它是没被移除的，则停止监听该channel
        this->update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(-1);//设置移除标识
}

void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    LOG_TRACE << "update channel fd:" << channel->fd();
    if (index == -1 || index == 2) {//如果它是被移除或被暂停监听的
        if (index == -1) {//如果是被移除的
            int fd = channel->fd();
            channels_[fd] = channel;//加回channels
        }
        channel->setIndex(1);//设置标识
        this->update(EPOLL_CTL_ADD, channel);//监听
    } else {//如果是正在监听的
        int fd = channel->fd();
        if (channel->isNoneEvent()) {//没发生事件，则暂停监听
            this->update(EPOLL_CTL_DEL, channel);
            channel->setIndex(2);
        } else {
            this->update(EPOLL_CTL_MOD, channel);//更新事件
        }
    }
}

Timestamp EPollPoller::poll(int timeout_ms, Poller::ChannelList *active_channels) {
    int num_events = epoll_wait(epoll_fd_, events_.data(), static_cast<int>(events_.size()), timeout_ms);
    int save_errno = errno;
    Timestamp now(Timestamp::now());
    if (num_events > 0) {//如果有事件发送
        LOG_TRACE << num_events << " events happened";
        fillActivateChannels(num_events, active_channels);//将发送事件的channel添加到active_channels，以供EventLoop使用
        if (num_events == events_.size()) {//如果所有事件都发送了，扩大epoll_wait返回的数量
            events_.resize(events_.size() * 2);
        }
    } else if (num_events == 0) {

    } else {
        if (save_errno != EINTR) {
            errno = save_errno;
        }
        LOG_ERROR << "EPollPoller::poll() error:" << strerror(errno);
    }
    return now;
}

void EPollPoller::fillActivateChannels(int num_events, Poller::ChannelList *activate_channels) const {
    for (int i = 0; i < num_events; ++i) {
        auto *channel = static_cast<Channel *>(events_[i].data.ptr);//从epoll中获取channel
        channel->setRevents(events_[i].events);//设置channel的revents
        activate_channels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel *channel) {
    epoll_event event{};
    int fd = channel->fd();
    //memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    if (::epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR << "epoll_ctl del error:" << strerror(errno);
        } else {
            LOG_FATAL << "epoll_ctl add/mod error:" << strerror(errno);
        }
    }
}
