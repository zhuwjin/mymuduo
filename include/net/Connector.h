#ifndef MYMUDUO_CONNECTOR_H
#define MYMUDUO_CONNECTOR_H

#include "Channel.h"
#include "InetAddress.h"
#include "base/noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>

class EventLoop;

class Connector : noncopyable, public std::enable_shared_from_this<Connector> {
public:
    using NewConnectionCallback = std::function<void(int)>;
    Connector(EventLoop *loop, const InetAddress &server_addr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb) {
        new_connection_callback_ = cb;
    }

    InetAddress serverAddress() const {
        return server_addr_;
    }

    void start();
    void restart();
    void stop();
private:
    enum State {
        Disconnected,
        Connecting,
        Connected
    };
    inline static const int MaxRetryDelayMs = 30 * 1000;
    inline static const int InitRetryDelayMs = 500;

    void setState(State s) {
        state_ = s;
    }

    void startInLoop();

    void stopInLoop();

    void connect();

    void connecting(int sockfd);

    void handleWrite();

    void handleError();

    void retry(int sockfd);

    int removeAndResetChannel();

    void resetChannel();

    EventLoop *loop_;
    InetAddress server_addr_;
    std::atomic_bool connect_;
    std::atomic_int state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback new_connection_callback_;
    int retry_delay_ms_;
};

#endif//MYMUDUO_CONNECTOR_H
