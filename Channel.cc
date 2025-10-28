#include "Channel.h"
#include "Timestamp.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;     
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;   
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd): loop_(loop), fd_(fd), events_(0), revents_(0), status_(-1), tied_(false){}

void Channel::update(){
    loop_->updateChannel(this);
}

void Channel::remove(){
    loop_->removeChannel(this);
}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvent(Timestamp recieveTime){
    if(tied_){
        auto guard = tie_.lock();
        if(guard){
            handleEventWithGuard(recieveTime);
        }
    }
    else{
        handleEventWithGuard(recieveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp recieveTime){

    LOG_INFO("channel handling revents, revents:%d\n", revents_);
    
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        if(closeCallback_)
            closeCallback_();
    }

    if(revents_ & EPOLLERR){
        if(errorCallback_)
            errorCallback_();
    }

    if(revents_ & EPOLLIN){
        if(readCallback_)
            readCallback_(recieveTime);
    }

    if(revents_ & EPOLLOUT){
        if(writeCallback_)
            writeCallback_();
    }
}

