#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <string.h>

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option):
    loop_(loop),
    ipPort_(listenAddr.toIpPort()),
    name_(name),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(),
    messageCallback_(),
    nextConnId_(1)
{
    auto removeConnection = [this](const TcpConnectionPtr& conn){
        auto removeConnectionInLoop = [this, &conn](){
            LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());
            connections_.erase(conn->name());
            EventLoop* ioLoop = conn->getLoop();
            ioLoop->runInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
        };
        loop_->runInLoop(removeConnectionInLoop);
    };

    auto newConnection = [this, removeConnection](int sockfd, const InetAddress& peerAddr){
        // 收到新连接 选择subloop
        EventLoop* ioLoop = threadPool_->getNextLoop();
        char buf[64] = {0};
        snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_++);
        std::string connName = name_ + buf;
        LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

        // 获取本地地址
        sockaddr_in local;
        memset(&local, 0, sizeof local);
        socklen_t addrlen = sizeof local;
        if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0){
            LOG_ERROR("%s %d sockets::getLocalAddr error\n", __FILE__, __LINE__);
        }
        InetAddress localAddr(local);

        // 创建TcpConnection
        TcpConnectionPtr conn = std::make_shared<TcpConnection>(ioLoop, connName, sockfd, localAddr, peerAddr);
        conn->test();
        connections_[connName] = conn;
        conn->test();

        //用户设置的回调 TcpServer -> TcpConnection -> Channel -> Poller -> notify Channel调用回调
        conn->setConnectionCallback(connectionCallback_);
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        conn->setCloseCallback(std::bind(removeConnection, std::placeholders::_1));
        conn->test();
        ioLoop->runInLoop(std::bind(&TcpConnection::connectionEstablished, conn.get()));
    };

    acceptor_->setNewConnectionCallback(newConnection);
}

void TcpServer::setThreadNum(int numThreads){ 
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start(){
    if(started_++ == 0){ // 防止被多次start
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

TcpServer::~TcpServer(){
    LOG_INFO("TcpServer::dtor() name:%s\n", name_.c_str());
    for(auto& item : connections_){
        TcpConnectionPtr guard(item.second);
        item.second.reset();
        guard->getLoop()->runInLoop(std::bind(&TcpConnection::connectionDestroyed, guard));
    }
}