#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/TcpClient.h"
#include <string>

int main(int argc, char **argv) {
    if (argc < 2) {
        LOG_FATAL << "usage:" << argv[0] << " <port>";
    }

    std::string data;
    for (unsigned long i = 0; i != 1024 * 1024; ++i) {
        data.push_back('a');
    }

    EventLoop loop;
    InetAddress server_addr(std::stoul(argv[1]));
    TcpClient client(&loop, server_addr, "echo_client");
    client.setConnectionCallback([&data](const TcpConnectionPtr &conn) {
        conn->send(data);
    });
    client.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf, Timestamp _t) {
        conn->send(buf->retrieveAllAsString());
    });

    client.connect();
    loop.loop();
    return 0;
}
