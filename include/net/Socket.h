#ifndef MYMUDUO_SOCKET_H
#define MYMUDUO_SOCKET_H

#include "InetAddress.h"
#include "base/Logging.h"
#include "SocketOps.h"
#include "base/noncopyable.h"
#include <fcntl.h>
#include <memory>
#include <netinet/tcp.h>
//禁止生成默认构造函数
class Socket : private noncopyable {
public:
    using ptr = std::shared_ptr<Socket>;

    explicit Socket(int fd) : fd_(fd) {}

    ~Socket() noexcept {
        SocketOps::close(fd_);
    }

    int getFd() const {
        return fd_;
    }

    void bind(const InetAddress &addr) const {
        SocketOps::bind(fd_, addr.getSockAddr());
    }

    void listen() const {
        SocketOps::listen(fd_);
    }

    void shutdownWrite() {
        SocketOps::shutdownWrite(fd_);
    }

    int accept(InetAddress *peeraddr) {
        sockaddr_in addr{};
        //memset(&addr, 0, sizeof(addr));
        int fd = SocketOps::accept(fd_, &addr);
        if (fd >= 0) {
            peeraddr->setSockAddr(addr);
        }
        return fd;
    }

    void setTcpNoDelay(bool on) {
        int optval = on ? 1 : 0;
        setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)));
    }

    void setReuseAddr(bool on) {
        int optval = on ? 1 : 0;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
    }

    void setReusePort(bool on) {
        int optval = on ? 1 : 0;
        int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0 && on) {
            LOG_ERROR << "setReusePort error!";
        }
    }

    void setKeepAlive(bool on) {
        int optval = on ? 1 : 0;
        setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
    }

    void setNonblocking() {
        fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK);
    }

private:
    int fd_;
};

#endif//MYMUDUO_SOCKET_H
