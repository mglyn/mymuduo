#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/errno.h>

static int creatNonblocking(){
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create error: %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd; 
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort):
    loop_(loop),
    acceptSocket_(creatNonblocking()),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.bindAdress(listenAddr);
    
    // TcpServer->start(); Acceptor->listen();
    auto handleRead = [this](){
        InetAddress peerAddr;
        int connfd = acceptSocket_.accept(&peerAddr);
        if(connfd >= 0){
            if(newConnectionCallback_){
                newConnectionCallback_(connfd, peerAddr); // 轮询得到subloop, 唤醒, 分发Channel
            }
            else{
                LOG_ERROR("%s:%s:%d accept but do nothing (no callback): %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
                ::close(connfd);
            }
        }
        else{
            LOG_ERROR("%s:%s:%d accept error: %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
            if(errno == EMFILE){
                LOG_ERROR("%s:%s:%d numSockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
            }
        }
    };

    acceptChannel_.setReadCallback(std::bind(handleRead));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}
