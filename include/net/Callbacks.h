#ifndef MYMUDUO_CALLBACKS_H
#define MYMUDUO_CALLBACKS_H

#include <functional>
#include <memory>

class Buffer;

class TcpConnection;

class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
using TimerCallback = std::function<void()>;

#endif//MYMUDUO_CALLBACKS_H
