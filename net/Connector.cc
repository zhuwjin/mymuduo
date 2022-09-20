#include "net/Connector.h"
#include "net/EventLoop.h"
#include "net/SocketOps.h"
#include "net/TimerId.h"

Connector::Connector(EventLoop *loop, const InetAddress &server_addr)
    : loop_(loop), server_addr_(server_addr),
      connect_(false), state_(Disconnected),
      retry_delay_ms_(InitRetryDelayMs) {
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
    LOG_DEBUG << "dtor[" << this << "]";
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop([this]() { startInLoop(); });
}

void Connector::startInLoop() {
    if (connect_) {
        connect();
    } else {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop([this]() { stopInLoop(); });
}

void Connector::stopInLoop() {
    if (state_ == Connecting) {
        setState(Disconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = SocketOps::createNonblockingSocket(server_addr_.family());
    int ret = SocketOps::connect(sockfd, server_addr_.getSockAddr());
    int saved_errno = (ret == 0) ? 0 : errno;
    switch (saved_errno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR << "connect error in Connector::startInLoop " << strerror(saved_errno);
            SocketOps::close(sockfd);
            break;
        default:
            LOG_ERROR << "Unexpected error in Connector::startInLoop " << strerror(saved_errno);
            SocketOps::close(sockfd);
            break;
    }
}

void Connector::restart() {
    setState(Disconnected);
    retry_delay_ms_ = InitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd) {
    setState(Connecting);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setErrorCallback([this] { handleError(); });
    channel_->tie(shared_from_this());
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop([this] { resetChannel(); });
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    LOG_TRACE << "Connector::handleWrite " << state_;
    if (state_ == Connecting) {
        int sockfd = removeAndResetChannel();
        int err = SocketOps::getSocketError(sockfd);
        if (err) {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror(err);
            retry(sockfd);
        } else if (SocketOps::isSelfConnect(sockfd)) {
            LOG_WARN << "Connector::handleWrite - Self connection";
            retry(sockfd);
        } else {
            setState(Connected);
            if (connect_) {
                new_connection_callback_(sockfd);
            } else {
                SocketOps::close(sockfd);
            }
        }
    } else {
        LOG_TRACE << "what happened?";
    }
}

void Connector::handleError() {
    LOG_ERROR << "Connector::handleError state=" << state_;
    if (state_ == Connecting) {
        int sockfd = removeAndResetChannel();
        int err = SocketOps::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    SocketOps::close(sockfd);
    setState(Disconnected);
    if (connect_) {
        LOG_INFO << "Connector::retry - Retry connecting to " << server_addr_.toIpPort()
                 << " in " << retry_delay_ms_ << " milliseconds. ";
        loop_->runAfter(retry_delay_ms_ / 1000.0, [p = shared_from_this()] { p->startInLoop(); });
        retry_delay_ms_ = std::min(retry_delay_ms_ * 2, MaxRetryDelayMs);
    } else {
        LOG_DEBUG << "do not connect";
    }
}
