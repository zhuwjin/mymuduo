#ifndef MYMUDUO_ACCEPTOR_H
#define MYMUDUO_ACCEPTOR_H

#include "base/noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>

class InetAddress;
class EventLoop;

/* listen和accept
 * 其中用Channel监听accept_socket的read事件，
 * 若有read事件说明有连接来了，则用accept接受信息，并生成与之连接的fd，再通过newConnectionCallback回调处理
 * */

class Acceptor : private noncopyable {
public:
    using NewConnectionCallback = std::function<void(int, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuse_port);

    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) {
        new_connection_callback_ = cb;
    }

    bool listening() const {
        return listening_;
    }

    void listen();

private:
    void handleRead();
    EventLoop *loop_;
    Socket accept_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listening_;
};

#endif//MYMUDUO_ACCEPTOR_H
