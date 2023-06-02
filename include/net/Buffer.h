#ifndef MYMUDUO_BUFFER_H
#define MYMUDUO_BUFFER_H

#include "../base/noncopyable.h"
#include "SocketOps.h"
#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

inline constexpr char CRLF[] = "\r\n";

class Buffer : public noncopyable {
public:
    using ptr = std::shared_ptr<Buffer>;
    static const size_t cheap_prepend = 8;
    static const size_t initial_size = 1024;
    explicit Buffer(size_t size = initial_size)
        : buffer_(initial_size + cheap_prepend),
          read_index_(cheap_prepend),
          writer_index_(cheap_prepend) {}

    size_t writeableBytes() const {
        return buffer_.size() - writer_index_;
    }

    size_t readableBytes() const {
        return writer_index_ - read_index_;
    }

    size_t prependableBytes() const {
        return read_index_;
    }

    void retrieveAll() {
        read_index_ = writer_index_ = cheap_prepend;
    }

    void retrieve(size_t len) {
        if (len < readableBytes()) {
            read_index_ += len;
        } else {
            retrieveAll();
        }
    }


    const char *peek() const {
        return begin() + read_index_;
    }

    const char *findCRLF() const {
        const char *crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    std::string retrieveAsString(size_t len) {
        std::string str(peek(), len);
        retrieve(len);
        return str;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    void append(const char *str, size_t len) {
        if (writeableBytes() < len) {
            makeSpace(len);
        }
        std::copy(str, str + len, beginWrite());
        writer_index_ += len;
    }

    void append(const void *str, size_t len) {
        append(static_cast<const char *>(str), len);
    }

    void append(const std::string &str) {
        append(str.c_str(), str.length());
    }


    char *beginWrite() {
        return begin() + writer_index_;
    }

    const char *beginWrite() const {
        return begin() + writer_index_;
    }

    ssize_t readFd(int fd, int *saved_errno) {
        char extrabuf[65536] = {0};
        const size_t writeable = writeableBytes();
        iovec vec[2];
        vec[0].iov_base = begin() + writer_index_;//第一块缓冲区
        vec[0].iov_len = writeable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof(extrabuf);
        const int iovcnt = writeable < sizeof(extrabuf) ? 2 : 1;
        const ssize_t n = SocketOps::readv(fd, vec, iovcnt);
        if (n < 0) {
            *saved_errno = errno;
        } else if (n <= writeable) {
            writer_index_ += n;
        } else {
            writer_index_ = buffer_.size();
            append(extrabuf, n - writeable);
        }
        return n;
    }

    ssize_t writeFd(int fd, int *saved_error) const {
        ssize_t len = SocketOps::write(fd, peek(), readableBytes());
        if (len < 0) {
            *saved_error = errno;
        }
        return len;
    }


private:
    void makeSpace(size_t len) {
        if (writeableBytes() + prependableBytes() < len + cheap_prepend) {
            buffer_.resize(writer_index_ + len);
        } else {
            size_t read_size = readableBytes();
            std::copy(begin() + read_index_, begin() + writer_index_, begin() + cheap_prepend);
            read_index_ = cheap_prepend;
            writer_index_ = read_index_ + read_size;
        }
    }
    char *begin() {
        return &*buffer_.begin();
    }

    const char *begin() const {
        return &*buffer_.cbegin();
    }
    std::vector<char> buffer_;
    std::atomic<size_t> read_index_;
    std::atomic<size_t> writer_index_;
};

#endif//MYMUDUO_BUFFER_H
