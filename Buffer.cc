#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 使用readv 可以填充到不连续的缓冲区
ssize_t Buffer::readFd(int fd, int* saveErrno){
    char extraBuf[65536] = {};
    struct iovec vec[2];
    size_t writable = writableBytes();
    
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof extraBuf;

    const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0){
        *saveErrno = errno;
    }
    else if(n <= writable){
        writerIndex_ += n;
    }
    else{
        writerIndex_ = buffer_.size();
        append(extraBuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno){
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0){
        *saveErrno = errno;
    }
    return n;
}
