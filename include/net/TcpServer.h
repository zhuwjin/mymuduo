#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "base/Timestamp.h"
#include "base/noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

#ifndef MYMUDUO_TCPSERVER_H
#define MYMUDUO_TCPSERVER_H


class TcpServer : private noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    enum Option {
        NoReusePort,
        ReusePort,
    };

    TcpServer(EventLoop *fd, const InetAddress &addr, std::string name, Option option = NoReusePort);

    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) {
        thread_init_callback_ = cb;
    }

    void setConnectionCallback(const ConnectionCallback &cb) {
        connection_callback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb) {
        message_callback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        write_complete_callback_ = cb;
    }

    void setThreadNum(int num);

    void start();

private:
    void newConnection(int sockfd, const InetAddress &peer_addr);

    void removeConnection(const TcpConnectionPtr &conn);

    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    EventLoop *loop_;
    const std::string ip_port_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> thread_pool_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    ThreadInitCallback thread_init_callback_;
    std::atomic_int started_;
    int next_conn_id_;
    ConnectionMap connections_;
};


#endif//MYMUDUO_TCPSERVER_H
