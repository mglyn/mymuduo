#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr):
    loop_(loop),
    name_(name),
    state_(kConnecting),
    reading_(false),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64 * 1024 * 1024)
{
    auto handleClose = [this](){
        LOG_INFO("%s %d Tcp handleClose fd: %d state: %d\n",__FILE__, __LINE__, channel_->fd(), (int)state_);
        setState(kDisconnected);
        channel_->disableAll();

        auto guardThis = shared_from_this();
        connectionCallback_(guardThis);
        closeCallback_(guardThis);
    };
    auto handleError = [this](){
        int optval;
        socklen_t optlen = sizeof optval;
        if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
            LOG_ERROR("%s %d Tcp handleError name: %s SO_ERROR:%d\n", __FILE__, __LINE__, name_.c_str(), errno);
        }
        else{
            LOG_ERROR("%s %d Tcp handleError name: %s SO_ERROR:%d\n", __FILE__, __LINE__, name_.c_str(), errno);
        }
    };
    auto handleRead = [this, handleClose, handleError](Timestamp recieveTime){
        int savedErrno = 0;
        ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
        if(n > 0){
            messageCallback_(shared_from_this(), &inputBuffer_, recieveTime);
        }
        else if(n == 0){
            handleClose();
        }
        else{
            errno = savedErrno;
            LOG_ERROR("TcpConnection::handleRead error\n");
            handleError();
        }
    };
    auto handleWrite = [this](){
        if(channel_->isWriting()){
            int savedErrno = 0;
            ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno); 
            if(n > 0){
                outputBuffer_.retrieve(n);
                // 全部发完 关闭写
                if(outputBuffer_.readableBytes() == 0){
                    channel_->disableReading();
                    if(writeCompleteCallback_){
                        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                    if(state_ == kDisconnecting){
                        shutdownInLoop();
                    }
                }
            }
            else{
                LOG_ERROR("TcpConnection handleWrite error, %d\n", savedErrno);
            }
        }
        else{
            LOG_ERROR("%s %d TcpConnection is not writing but handleWrite being called\n", __FILE__, __LINE__);
        }
    };

    channel_->setReadCallback(std::bind(handleRead, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(handleWrite));
    channel_->setCloseCallback(std::bind(handleClose));
    channel_->setErrorCallback(std::bind(handleError));

    LOG_INFO("TcpConnection::ctor[%s] at fd: %d\n", name.c_str(), sockfd);
    socket_->setKeepAlive(true);

}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor[%s] at fd: %d state: %d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::sendInLoop(const void* data, size_t len){
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if(state_ == kDisconnected){
        LOG_ERROR("disconnected... give up writing\n");
        return;
    }
    // channel第一次写数据 直接写入
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
        LOG_DEBUG("sendInLoop sending..\n");
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote > 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else{
            nwrote = 0;
            if(errno != EWOULDBLOCK){
                LOG_ERROR("%s %s %d error\n", __FUNCTION__, __FILE__, __LINE__);
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }

    // 当前这一次write没发完全部数据 需要保存到缓冲 并注册下次写事件
    if(!faultError && remaining > 0){
        size_t oldLen = outputBuffer_.readableBytes();
        // 缓冲区即将触发高水位预警
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char*)data + nwrote, remaining);

        // 启用写 handleWrite回调会发送剩下数据
        if(!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}

void TcpConnection::send(const std::string& str){
    if(state_ == kConnected){
        loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, str.c_str(), str.size()));
    }
}

void TcpConnection::connectionEstablished(){
    setState(kConnected);
    test();
    channel_->tie(shared_from_this());
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectionDestroyed(){
    if(state_ == kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_->isWriting()){
        socket_->shutdownWrite();
    }
}

void TcpConnection::test(){
    auto pp = shared_from_this();
}


