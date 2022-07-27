#include "net/Channel.h"
#include "net/EventLoop.h"

void Channel::enableReading() {
    events_ |= ReadEvent;
    loop_->updateChannel(this);
}

void Channel::disableReading() {
    events_ &= ~ReadEvent;
    loop_->updateChannel(this);
}

void Channel::enableWriting() {
    events_ |= WriteEvent;
    loop_->updateChannel(this);
}

void Channel::disableWriting() {
    events_ &= ~WriteEvent;
    loop_->updateChannel(this);
}

void Channel::disableAll() {
    events_ = NoneEvent;
}

void Channel::remove() {
    loop_->removeChannel(this);
}