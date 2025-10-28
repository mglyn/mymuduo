#pragma once

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "noncopyable.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "TcpConnection.h"

#include <functional>
#include <string>
#include <memory>
#include <map>
#include <atomic>

//对外使用的类
class TcpServer: noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option{
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb){threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}

    void setThreadNum(int numThreads);
    void start(); // 开启服务监听

private:
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;           // baseloop 用户定义的loop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop 监听新连接事件
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_;
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // 保存所有链接
};
