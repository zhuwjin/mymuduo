#ifndef MYMUDUO_INETADDRESS_H
#define MYMUDUO_INETADDRESS_H

#include "base/copyable.h"
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <string>

class InetAddress : public copyable {
public:
    using ptr = std::shared_ptr<InetAddress>;

    explicit InetAddress() = default;

    explicit InetAddress(uint16_t port, const std::string &ip = "127.0.0.1") : addr_({}) {
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = inet_addr(ip.c_str());
        addr_.sin_port = htons(port);
    }

    InetAddress(const std::string &ip, uint16_t port) : addr_({}) {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    }

    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    const sockaddr *getSockAddr() const {
        return reinterpret_cast<const sockaddr *>(&addr_);
    }

    void setSockAddr(const sockaddr_in &addr) {
        addr_ = addr;
    }

    sa_family_t family() const {
        return addr_.sin_family;
    }

    std::string getIp() const {
        char buf[64];
        inet_ntop(AF_INET, &addr_.sin_port, buf, static_cast<socklen_t>(sizeof(addr_)));
        return buf;
    }

    uint16_t getPort() const {
        return be16toh(addr_.sin_port);
    }

    std::string toIpPort() const {
        char buf[64] = {0};
        inet_ntop(AF_INET, &addr_.sin_port, buf, static_cast<socklen_t>(sizeof(addr_)));
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%u", this->getPort());
        return buf;
    }

private:
    sockaddr_in addr_;
};

#endif//MYMUDUO_INETADDRESS_H
