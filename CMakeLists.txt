cmake_minimum_required(VERSION 3.25)
project(mymuduo)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(mymuduo net/SocketOps.cc net/Poller.cc net/EPollPoller.cc net/EventLoop.cc net/Channel.cc net/EventLoopThread.cc net/Acceptor.cc net/TcpConnection.cc net/TcpServer.cc net/EventLoopThreadPool.cc net/Connector.cc net/TimerQueue.cc net/TcpClient.cc)

add_executable(test1 test.cc)
target_link_libraries(test1 mymuduo)
