#ifndef MYMUDUO_TCPCLIENT_H
#define MYMUDUO_TCPCLIENT_H

#include "base/noncopyable.h"
#include "net/TcpConnection.h"
#include <atomic>
#include <mutex>

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable {
public:
    TcpClient(EventLoop *loop, const InetAddress &server_addr, const std::string &name);
    ~TcpClient();
    void connect();
    void disconnect();
    void stop();
    TcpConnectionPtr connection() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return connection_;
    }
    EventLoop *getLoop() const { return loop_; }

    bool retry() const { return retry_; }

    void enableRetry() { retry_ = true; }

    const std::string &name() const {
        return name_;
    }

    void setConnectionCallback(ConnectionCallback cb) {
        connection_callback_ = std::move(cb);
    }

    void setMessageCallback(MessageCallback cb) {
        message_callback_ = std::move(cb);
    }

    void setWriteCompleteCallback(WriteCompleteCallback cb) {
        write_complete_callback_ = std::move(cb);
    }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr &conn);
    EventLoop *loop_;
    ConnectorPtr connector_;
    const std::string name_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    std::atomic_bool retry_;
    std::atomic_bool connect_;
    int next_connid_;
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_;
};

#endif//MYMUDUO_TCPCLIENT_H
