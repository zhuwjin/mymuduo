#include "net/SocketOps.h"
#include "base/Logging.h"
#include <cstring>


namespace SocketOps {
    int createNonblockingSocket(sa_family_t family) {
        int fd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if (fd < 0) {
            LOG_FATAL << "Sockets::createNonblockingSocket:" << strerror(errno);
        }
        return fd;
    }

    int connect(int fd, const sockaddr *addr) {
        return ::connect(fd, addr, sizeof(sockaddr_in));
    }

    void bind(int fd, const sockaddr *addr) {
        if (::bind(fd, addr, sizeof(sockaddr_in)) < 0) {
            LOG_FATAL << "Sockets::bind:" << strerror(errno);
        }
    }

    void listen(int fd) {
        if (::listen(fd, SOMAXCONN) < 0) {
            LOG_FATAL << "Sockets::listen:" << strerror(errno);
        }
    }

    int accept(int fd, sockaddr_in *addr) {
        auto addrlen = static_cast<socklen_t>(sizeof(*addr));
        int coonfd = ::accept4(fd, reinterpret_cast<sockaddr *>(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (coonfd < 0) {
            LOG_FATAL << "Socket::accept";
            int no = errno;
            switch (no) {
                case EMFILE:
                    errno = no;
                    break;
                case EOPNOTSUPP:
                    LOG_FATAL << "unexpected error of ::accept:" << strerror(no);
                    break;
            }
        }
        return coonfd;
    }

    void close(int fd) {
        if (::close(fd) < 0) {
            LOG_FATAL << "Sockets::close:" << strerror(errno);
        }
    }

    void shutdownWrite(int fd) {
        if (::shutdown(fd, SHUT_WR) < 0) {
            LOG_FATAL << "Sockets::shutdownWrite:" << strerror(errno);
        }
    }

    ssize_t readv(int fd, const iovec *iov, int iovcnt) {
        return ::readv(fd, iov, iovcnt);
    }

    ssize_t write(int fd, const void *buf, size_t size) {
        return ::write(fd, buf, size);
    }

    int getSocketError(int sockfd) {
        int optval;
        auto optlen = static_cast<socklen_t>(sizeof optval);
        if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
            return errno;
        } else {
            return optval;
        }
    }

    struct sockaddr_in getLocalAddr(int sockfd) {
        struct sockaddr_in localaddr {};
        auto addrlen = static_cast<socklen_t>(sizeof localaddr);
        if (::getsockname(sockfd, reinterpret_cast<sockaddr *>(&localaddr), &addrlen) < 0) {
            LOG_ERROR << "sockets::getLocalAddr:" << strerror(errno);
        }
        return localaddr;
    }

    struct sockaddr_in getPeerAddr(int sockfd) {
        struct sockaddr_in peeraddr{};
        auto addrlen = static_cast<socklen_t>(sizeof peeraddr);
        if (::getpeername(sockfd, reinterpret_cast<sockaddr *>(&peeraddr), &addrlen) < 0)
        {
            LOG_ERROR << "sockets::getPeerAddr:" << strerror(errno);
        }
        return peeraddr;
    }


    bool isSelfConnect(int sockfd) {
        struct sockaddr_in localaddr = getLocalAddr(sockfd);
        struct sockaddr_in peeraddr = getPeerAddr(sockfd);
        if (localaddr.sin_family == AF_INET) {
            const struct sockaddr_in *laddr4 =  &localaddr;
            const struct sockaddr_in *raddr4 = &peeraddr;
            return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
        } else {
            return false;
        }
    }
}// namespace SocketOps