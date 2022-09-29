#ifndef MYMUDUO_SOCKETOPS_H
#define MYMUDUO_SOCKETOPS_H

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/uio.h>

namespace SocketOps {
    int createNonblockingSocket(sa_family_t family);

    int connect(int fd, const sockaddr *addr);

    void bind(int fd, const sockaddr *addr);

    void listen(int fd);

    int accept(int fd, sockaddr_in *addr);

    void close(int fd);

    void shutdownWrite(int fd);

    int getSocketError(int sockfd);

    ssize_t readv(int fd, const iovec *iov, int iovcnt);

    ssize_t write(int fd, const void *buf, size_t size);

    bool isSelfConnect(int sockfd);

    struct sockaddr_in getLocalAddr(int sockfd);

    struct sockaddr_in getPeerAddr(int sockfd);

}// namespace SocketOps

#endif//MYMUDUO_SOCKETOPS_H
