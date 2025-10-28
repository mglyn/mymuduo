#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor{

public:
    using newConnectionCallback = std::function<void(int sockfd, const InetAddress&)>; 
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(const newConnectionCallback& cb){newConnectionCallback_ = cb;}
    bool listenning() const {return listenning_;}
    void listen();

private:
    EventLoop* loop_; // baseloop
    Socket  acceptSocket_;
    Channel acceptChannel_;

    newConnectionCallback newConnectionCallback_;
    bool listenning_;
};