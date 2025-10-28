#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include "Timestamp.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop* loop): 
ownerLoop_(loop), 
epollfd_(epoll_create1(EPOLL_CLOEXEC)),
events_(kInitEventListSize){

    if(epollfd_ < 0){
        LOG_FATAL("epoll_create error: %d\n", errno);
    }
}

EpollPoller::~EpollPoller(){
    close(epollfd_);
}

Timestamp EpollPoller::poll(int timeOutMiliseconds, std::vector<Channel*>* activeChannels){
    LOG_DEBUG("poll => fd total count: %lu\n", channels_.size());
    int numEvents = epoll_wait(epollfd_, &(*events_.begin()), static_cast<int>(events_.size()), timeOutMiliseconds);
    Timestamp now(Timestamp::now());

    if(numEvents > 0){
        LOG_INFO("poll => %d events happened\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == (int)events_.size()){
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0){
        LOG_DEBUG("poll => timeout\n");
    }
    else{
        LOG_ERROR("poll => %d error", errno);
    }

    return now;
}

void EpollPoller::updateChannel(Channel* channel){
    const int status = channel->status();
    LOG_INFO("updateChannel => fd = %d events = %d status = %d\n", channel->fd(), channel->events(), status);

    if(status == kNew || status == kDeleted){
        if(status == kNew){
            channels_[channel->fd()] = channel;
        }

        channel->set_status(kAdded);
        update(EPOLL_CTL_ADD, channel); 
    }
    else{ // status == added
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_status(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel){
    LOG_INFO("removeChannel => fd = %d events = %d status = %d\n", channel->fd(), channel->events(), channel->status());
    channels_.erase(channel->fd());

    if(channel->status() == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_status(kNew);
}

bool EpollPoller::hasChannel(Channel* channel) const {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel; 
}

void EpollPoller::fillActiveChannels(int numEvents, std::vector<Channel*>* activeChannels){
    for(int i = 0; i < numEvents; i++){
        Channel* channel  = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel* channel){
    epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;

    if(epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        }
        else{
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}
