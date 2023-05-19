#include "net/EventLoop.h"
#include "net/TcpServer.h"

class EchoServer {
public:
    EchoServer(EventLoop *loop, const InetAddress &addr) : loop_(loop), server_(loop, addr, "EchoServer") {
        server_.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf, Timestamp t) {
            conn->send(buf->retrieveAllAsString());
        });
        server_.setConnectionCallback([](const TcpConnectionPtr &conn) {
            LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
                     << conn->localAddress().toIpPort() << " is " << (conn->connect() ? "UP" : "DOWN");
        });
    }

    void start() {
        server_.start();
    }

private:
    EventLoop *loop_;
    TcpServer server_;
};


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "usage:" << argv[0] << " port" << std::endl;
        return 2;
    }
    auto port = std::stoul(argv[1]);
    LOG_INFO << "listen port:" << port;
    EventLoop loop;
    EchoServer server(&loop, InetAddress(port));
    server.start();
    loop.loop();
}