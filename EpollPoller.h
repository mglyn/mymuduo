#pragma once

#include "noncopyable.h"

#include <vector>
#include <map>
#include <sys/epoll.h>

class Channel;
class EventLoop;
class Timestamp;

class EpollPoller: noncopyable{
public:
    EpollPoller(EventLoop* loop);
    
    ~EpollPoller();

    // IO复用接口
    Timestamp poll(int timeOutMiliseconds, std::vector<Channel*>* channels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel) const;

private:
    // 填写活跃的链接
    void fillActiveChannels(int numEvents, std::vector<Channel*>* activeChannels);
    // 更新Channel
    void update(int operation, Channel* channel);

    EventLoop* ownerLoop_;
    // fd, Channel
    std::map<int, Channel*> channels_;

    static const int kInitEventListSize = 16;

    int epollfd_;
    std::vector<epoll_event> events_; 
};