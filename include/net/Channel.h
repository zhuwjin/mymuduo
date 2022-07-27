#ifndef MYMUDUO_CHANNEL_H
#define MYMUDUO_CHANNEL_H

#include "base/Logging.h"
#include "base/Timestamp.h"
#include "base/noncopyable.h"
#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;

/* 用Channel管理fd的事件
 * 设置事件的回调函数
 * 有events_和revents，通过Poller设置revents，若有事件发生则调用handleEvent，根据类型执行回调
 * */

class Channel : private noncopyable {
public:
    using ptr = std::shared_ptr<Channel>;
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;


    Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd),
                                       events_(0), revents_(0),
                                       index_(-1), tied_(false) {}

    void setReadCallback(ReadEventCallback cb) {
        this->read_callback_ = std::move(cb);
    }

    void setWriteCallback(EventCallback cb) {
        this->write_callback_ = std::move(cb);
    }

    void setCloseCallback(EventCallback cb) {
        this->close_callback_ = std::move(cb);
    }

    void setErrorCallback(EventCallback cb) {
        this->error_callback_ = std::move(cb);
    }

    bool isNoneEvent() const {
        return this->events_ == this->NoneEvent;
    }

    bool isWriting() const {
        return this->events_ & this->WriteEvent;
    }

    bool isReading() const {
        return this->events_ & this->ReadEvent;
    }

    void enableReading();

    void disableReading();

    void enableWriting();

    void disableWriting();

    void disableAll();

    uint32_t revents() const {
        return this->revents_;
    }

    void setRevents(uint32_t revt) {
        this->revents_ = revt;
    }

    int index() const {
        return this->index_;
    }

    void setIndex(int index) {
        this->index_ = index;
    }

    uint32_t events() const {
        return this->events_;
    }

    int fd() const {
        return this->fd_;
    }

    void remove();

    void tie(const std::shared_ptr<void> &obj) {
        this->tie_ = obj;
        this->tied_ = true;
    }


    void handleEvent(Timestamp receive_time) {
        if (this->tied_) {
            std::shared_ptr<void> guard = this->tie_.lock();
            if (guard) {
                this->handleEventWithGuard(receive_time);
            }
        } else {
            this->handleEventWithGuard(receive_time);
        }
    }


private:
    void handleEventWithGuard(Timestamp receive_time) {
        LOG_TRACE << "channel handleEvent revents:" << revents();
        if ((this->revents_ & EPOLLHUP) && !(this->revents_ & EPOLLIN)) {
            if (this->close_callback_) {
                this->close_callback_();
            }
        }
        if (this->revents_ & EPOLLERR) {
            if (this->error_callback_) {
                this->error_callback_();
            }
        }
        if (this->revents_ & (EPOLLIN | EPOLLPRI)) {
            if (this->read_callback_) {
                this->read_callback_(receive_time);
            }
        }
        if (this->revents_ & EPOLLOUT) {
            if (this->write_callback_) {
                this->write_callback_();
            }
        }
    }
    static const int NoneEvent = 0;
    static const int ReadEvent = EPOLLIN | EPOLLET;
    static const int WriteEvent = EPOLLOUT;

    EventLoop *loop_;
    const int fd_;
    uint32_t events_;
    uint32_t revents_;
    int index_;
    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

#endif//MYMUDUO_CHANNEL_H
