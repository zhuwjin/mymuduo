#include "net/EventLoop.h"
#include "net/Channel.h"
#include "net/EPollPoller.h"
#include "net/Poller.h"
#include "net/TimerId.h"
#include "net/TimerQueue.h"
#include <cstring>
#include <sys/eventfd.h>
#include <unistd.h>

thread_local EventLoop *t_loopInThisThread = nullptr;

const int PollTimeMs = 10000;

EventLoop::EventLoop() : looping_(false), quit_(false),
                         calling_pending_functions_(false),
                         poller_(new EPollPoller(this)),
                         timer_queue_(new TimerQueue(this)),
                         thread_id_(std::this_thread::get_id()) {
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL << "eventfd error:" << strerror(errno);
    }
    this->wakeup_fd_ = evtfd;
    this->wakeup_channel_ = std::make_unique<Channel>(this, wakeup_fd_);
    LOG_DEBUG << "EventLoop created " << this << " in base" << thread_id_;
    if (t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this base " << thread_id_;
    } else {
        t_loopInThisThread = this;
    }

    wakeup_channel_->setReadCallback([this](Timestamp) { handleRead(); });
    wakeup_channel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeup_channel_->disableAll();
    wakeup_channel_->remove();
    close(wakeup_fd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";
    while (!quit_) {
        active_channels_.clear();
        poll_return_time_ = poller_->poll(PollTimeMs, &active_channels_);
        for (Channel *channel: active_channels_) {
            channel->handleEvent((poll_return_time_));
        }
        this->doPendingFunctors();
    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!this->isInLoopThread()) {
        this->wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
    return timer_queue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
    Timestamp time(Timestamp::now() + delay);
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time(Timestamp::now() + interval);
    return timer_queue_->addTimer(cb, time, interval);
}


void EventLoop::runInLoop(Functor cb) {
    if (this->isInLoopThread()) {
        cb();
    } else {
        this->queueInLoop(cb);
    }
}

bool EventLoop::isInLoopThread() const {
    return this->thread_id_ == std::this_thread::get_id();
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> _lock(this->mutex_);
        pending_functors_.push_back(cb);
    }
    if (!this->isInLoopThread() || this->calling_pending_functions_) {
        this->wakeup();
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel *channel) {
    this->poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    this->poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return this->poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    calling_pending_functions_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }
    for (const Functor &functor: functors) {
        functor();
    }
    calling_pending_functions_ = false;
}