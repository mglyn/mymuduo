#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket: noncopyable{

public:
    explicit Socket(int fd): sockfd_(fd){}
    ~Socket();

    int fd() const {return sockfd_;}
    void bindAdress(const InetAddress& localAddr);
    void listen();
    int accept(InetAddress* peerAddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};