#include "net/TcpServer.h"
#include "net/EventLoopThreadPool.h"



TcpServer::TcpServer(EventLoop *loop, const InetAddress &listen_addr,
                     std::string name, TcpServer::Option option)
    : loop_(loop), ip_port_(listen_addr.toIpPort()),
      name_(std::move(name)), acceptor_(new Acceptor(loop, listen_addr, option == ReusePort)),
      thread_pool_(new EventLoopThreadPool(loop, name_)),
      connection_callback_(defaultConnectionCallback),
      message_callback_(defaultMessageCallback),
      write_complete_callback_(),
      next_conn_id_(1),
      started_(0) {
    acceptor_->setNewConnectionCallback([this](auto &&fd, auto &&addr) { newConnection(std::forward<decltype(fd)>(fd), std::forward<decltype(addr)>(addr)); });
}

TcpServer::~TcpServer() {
    for (auto &item: connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop([conn]() mutable { conn->connectDestroyed(); });
    }
}

void TcpServer::start() {
    if (started_++ == 0) {
        thread_pool_->start(thread_init_callback_);
        loop_->runInLoop([acceptor = acceptor_.get()] { acceptor->listen(); });
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peer_addr) {
    EventLoop *io_loop = thread_pool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ip_port_.c_str(), next_conn_id_);
    ++next_conn_id_;
    std::string conn_name = name_ + buf;
    LOG_TRACE << "TcpServer::newConnection " << name_ << " - new connection " << conn_name << " from %s" << peer_addr.toIpPort();
    sockaddr_in local{};
    //memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (getsockname(sockfd, (sockaddr *) &local, &addrlen) < 0) {
        LOG_ERROR << "sockets::getLocalAddr:" << strerror(errno);
    }
    InetAddress local_addr(local);
    TcpConnectionPtr conn(new TcpConnection(io_loop, conn_name, sockfd, local_addr, peer_addr));
    connections_[conn_name] = conn;
    conn->setConnectionCallback(connection_callback_);
    conn->setMessageCallback(message_callback_);
    conn->setWriteCompleteCallback(write_complete_callback_);
    conn->setCloseCallback([this](const TcpConnectionPtr &conn_ptr) { removeConnection(conn_ptr); });
    io_loop->runInLoop([conn]() mutable { conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop([this, conn] { removeConnectionInLoop(conn); });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    LOG_TRACE << "TcpServer::removeConnectionInLoop " << name_.c_str() << " - connection " << conn->name();
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop([conn] () mutable { conn->connectDestroyed(); });
}
void TcpServer::setThreadNum(int num) {
    thread_pool_->setThreadNum(num);
}
