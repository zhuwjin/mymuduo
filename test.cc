#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/TcpClient.h"
#include <iostream>

const std::string data = "hello world";

int main() {
    //Logger::setGLevel(Logger::TRACE);
    EventLoop loop;
    InetAddress server_addr(12345);
    TcpClient client(&loop, server_addr, "echo-client");
    client.setConnectionCallback([](const TcpConnectionPtr &conn) {
        conn->send(data);
    });
    client.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf, Timestamp t) {
        conn->send(buf->retrieveAllAsString());
        //std::cout << buf->retrieveAllAsString() << std::endl;
    });
    client.connect();
    loop.loop();
}
