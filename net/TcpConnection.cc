#include "net/TcpConnection.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include <utility>

void defaultConnectionCallback(const TcpConnectionPtr &conn) {
    LOG_TRACE << "new connect";
}

void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time) {
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, std::string name,
                             int sockfd, const InetAddress &local_addr,
                             const InetAddress &peer_addr)
    : loop_(loop), name_(std::move(name)), state_(Connecting),
      reading_(true), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      local_addr_(local_addr), peer_addr_(peer_addr),
      high_water_mark_(64 * 1024 * 1024) {
    channel_->setReadCallback([this](auto &&t) { handleRead(std::forward<decltype(t)>(t)); });
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setErrorCallback([this] { handleError(); });
    channel_->setCloseCallback([this] { handleClose(); });
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() = default;

void TcpConnection::send(const std::string &buf) {
    if (state_ == Connected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf);
        } else {
            loop_->runInLoop([this, buf] { sendInLoop(buf); });
        }
    }
}

void TcpConnection::sendInLoop(const std::string &msg) {
    ssize_t nwrote = 0;
    size_t remaining = msg.size();
    bool fault_error = false;

    if (state_ == Disconnected) {
        return;
    }
    if (!channel_->isWriting() && output_buffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), msg.c_str(), msg.size());
        if (nwrote >= 0) {
            remaining = msg.size() - nwrote;
            if (remaining == 0 && write_complete_callback_) {
                loop_->queueInLoop([this] { write_complete_callback_(shared_from_this()); });
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR << "TcpConnection::sendInLoop:" << strerror(errno);
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true;
                }
            }
        }
    }

    if (!fault_error && remaining > 0) {
        size_t old_len = output_buffer_.readableBytes();
        if (old_len + remaining >= high_water_mark_ && old_len < high_water_mark_ && high_water_mark_callback_) {
            loop_->queueInLoop(std::bind(high_water_mark_callback_, shared_from_this(), old_len + remaining));
        }
        output_buffer_.append(msg.c_str() + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == Connected) {
        setState(Disconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished() {
    setState(Connected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    connection_callback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    if (state_ == Connected) {
        setState(Disconnected);
        channel_->disableAll();
        connection_callback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receive_time) {
    int saved_errno;
    ssize_t n = input_buffer_.readFd(channel_->fd(), &saved_errno);
    if (n > 0) {
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    } else if (n == 0) {
        this->handleClose();
    } else {
        errno = saved_errno;
        LOG_ERROR << "TcpConnection::HandleRead";
        this->handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int saved_errno = 0;
        ssize_t n = output_buffer_.writeFd(channel_->fd(), &saved_errno);
        if (n > 0) {
            output_buffer_.retrieve(n);
            if (output_buffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (write_complete_callback_) {
                    loop_->queueInLoop(std::bind(write_complete_callback_, shared_from_this()));
                }
                if (state_ == Disconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    } else {
        LOG_ERROR << "TcpConnection fd=" << channel_->fd() << " is down";
    }
}

void TcpConnection::handleClose() {
    LOG_TRACE << "TcpConnection::handleClose fd=" << channel_->fd() << " state=" << state_;
    setState(Disconnected);
    channel_->disableAll();
    TcpConnectionPtr conn_ptr(shared_from_this());
    connection_callback_(conn_ptr);
    close_callback_(conn_ptr);
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen)) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError name:" << name_ << " - SO_ERROR:" << strerror(err);
}
void TcpConnection::forceClose() {
    if (state_ == Connected || state_ == Disconnecting) {
        setState(Disconnecting);
        loop_->queueInLoop([p = shared_from_this()] { p->forceCloseInLoop(); });
    }
}
void TcpConnection::forceCloseInLoop() {
    if (state_ == Connected || state_ == Disconnecting) {
        handleClose();
    }
}
