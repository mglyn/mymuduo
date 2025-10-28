#pragma once

#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;
class Timestamp;

/*
    T]tcpServer => tcpconnection设置回调 => channel设置回调 => poller => channel回调
*/
class TcpConnection: public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}

    bool connected() const {return state_ == kConnected;};
    bool disconnected() const {return state_ == kDisconnected;};

    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark){
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb){closeCallback_ = cb;}

    void connectionEstablished();
    void connectionDestroyed();
    void sendInLoop(const void* message, size_t len);
    void shutdown();
    void send(const std::string& str);
    void test();

private:
    enum StateE{
        kDisconnected, 
        kConnecting,
        kConnected,
        kDisconnecting
    };

    EventLoop* loop_;  // 某subloop
    std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    void setState(StateE state){state_ = state;}
    void shutdownInLoop();
};