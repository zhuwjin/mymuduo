#include "net/Acceptor.h"
#include "net/SocketOps.h"

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuse_port)
    : loop_(loop), listening_(false),
      accept_socket_(SocketOps::createNonblockingSocket(listen_addr.family())),
      accept_channel_(loop, accept_socket_.getFd()) {
    accept_socket_.setReuseAddr(true);
    accept_socket_.setReusePort(reuse_port);
    accept_socket_.bind(listen_addr);
    accept_channel_.setReadCallback([this](Timestamp _) { handleRead(); });
}

Acceptor::~Acceptor() {
    accept_channel_.disableAll();
    accept_channel_.remove();
}

void Acceptor::listen() {
    listening_ = true;
    accept_socket_.listen();
    accept_channel_.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peer_addr{};
    int connfd = accept_socket_.accept(&peer_addr);
    if (connfd >= 0) {
        if (new_connection_callback_) {
            new_connection_callback_(connfd, peer_addr);
        } else {
            ::close(connfd);
        }
    } else {
        LOG_ERROR << "accept error:" << strerror(errno);
        if (errno == EMFILE) {
            LOG_ERROR << "sockfd reached limit:" << strerror(errno);
        }
    }
}
