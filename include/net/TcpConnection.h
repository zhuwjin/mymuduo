#ifndef MYMUDUO_TCPCONNECTION_H
#define MYMUDUO_TCPCONNECTION_H

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "base/noncopyable.h"
#include <any>
#include <atomic>
#include <utility>

class EventLoop;

class Socket;

class Channel;

class TcpConnection : private noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop *t, std::string name, int sockfd,
                  const InetAddress &local_addr,
                  const InetAddress &peer_addr);
    ~TcpConnection();

    void setConnectionCallback(const ConnectionCallback &cb) {
        connection_callback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb) {
        message_callback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        write_complete_callback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t high_water_mark) {
        high_water_mark_callback_ = cb;
        high_water_mark_ = high_water_mark;
    }

    void setCloseCallback(const CloseCallback &cb) {
        close_callback_ = cb;
    }

    const std::string &name() const {
        return name_;
    }

    EventLoop *getLoop() const {
        return loop_;
    }

    void send(const std::string &buf);

    void shutdown();

    void forceClose();

    void forceCloseInLoop();

    bool connect() const {
        return state_ == Connected;
    }

    const InetAddress &localAddress() const {
        return local_addr_;
    }

    const InetAddress &peerAddress() const {
        return peer_addr_;
    }

    void connectEstablished();

    void connectDestroyed();

    void setContext(std::any context) {
        this->context_ = std::move(context);
    }

    const std::any *getContext() const {
        return &context_;
    }

    std::any *getMutableContext() {
        return &context_;
    }

private:
    enum StateE {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };

    void setState(StateE state) {
        state_ = state;
    }

    void handleRead(Timestamp receive_time);

    void handleWrite();

    void handleClose();

    void handleError();

    void sendInLoop(const std::string &msg);

    void shutdownInLoop();

    EventLoop *loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress local_addr_;
    const InetAddress peer_addr_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_water_mark_callback_;
    CloseCallback close_callback_;
    size_t high_water_mark_;
    Buffer input_buffer_;
    Buffer output_buffer_;
    std::any context_;
};

void defaultConnectionCallback(const TcpConnectionPtr &conn);

void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time);

#endif//MYMUDUO_TCPCONNECTION_H
