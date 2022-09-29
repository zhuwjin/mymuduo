#include "net/TcpClient.h"
#include "base/Logging.h"
#include "net/Connector.h"
#include "net/EventLoop.h"
#include "net/SocketOps.h"
#include <cstdio>


void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn) {
    loop->queueInLoop([conn] { conn->connectDestroyed(); });
}

void removeConnector(const ConnectorPtr &connector) {
}

TcpClient::TcpClient(EventLoop *loop, const InetAddress &server_addr, const std::string &name)
    : loop_(loop), connector_(new Connector(loop, server_addr)),
      name_(name), connection_callback_(defaultConnectionCallback),
      message_callback_(defaultMessageCallback),
      retry_(false), connect_(true),
      next_connid_(1) {
    connector_->setNewConnectionCallback([this](int fd) { newConnection(fd); });
    LOG_INFO << "TcpClient::TcpClient[" << name_ << "] - connector" << connector_.get();
}

TcpClient::~TcpClient() {
    LOG_INFO << "TcpClient::TcpClient[" << name_ << "] - connector" << connector_.get();
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn) {
        CloseCallback cb = [this](const TcpConnectionPtr &conn) { removeConnection(conn); };
        loop_->runInLoop([conn, cb] { conn->setCloseCallback(cb); });
        if (unique) {
            conn->forceClose();
        }
    } else {
        connector_->stop();
        loop_->runAfter(1, [this] { removeConnector(this->connector_); });
    }
}

void TcpClient::connect() {
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connection to "
             << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
    InetAddress peer_addr(SocketOps::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peer_addr.toIpPort().c_str(), next_connid_);
    ++next_connid_;
    std::string conn_name = name_ + buf;
    InetAddress local_addr(SocketOps::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(loop_, conn_name, sockfd, local_addr, peer_addr));
    conn->setConnectionCallback(connection_callback_);
    conn->setMessageCallback(message_callback_);
    conn->setWriteCompleteCallback(write_complete_callback_);
    conn->setCloseCallback([this](const TcpConnectionPtr &conn) { removeConnection(conn); });
    {
        std::lock_guard<std::mutex> lk(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        connection_.reset();
    }
    loop_->queueInLoop([conn] { conn->connectDestroyed(); });
    if (retry_ && connect_) {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                 << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}