#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Timestamp.h"

class Channel;
class Poller;
class Timestamp;
class EpollPoller;

//事件循环
class EventLoop: noncopyable{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop(); // 开启事件循环
    void quit(); // 退出事件循环

    Timestamp pollReturnTime() const {return pollReturnTime_;} 

    void runInLoop(Functor cb);   // 在当前loop中执行cb
    void queueInLoop(Functor cb); // cb入队， 唤醒loop所在线程执行cb

    void wakeup(); // 唤醒loop所在线程

    // 调用poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断当前loop是否执行在其自己的线程里
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

private:
    void doPendingFunctors(); 

    using ChannelList = std::vector<Channel*>;
    
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    const pid_t threadId_; // 记录当前loop所在线程的ID
    Timestamp pollReturnTime_; // poller返回发生事件Channel的时间点
    std::unique_ptr<EpollPoller> poller_;
    
    int wakeupFd_; // mainloop 获取到一个新用户的Channel， 轮询选择一个subloop， 改成员唤醒subloop
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic<bool> callingPendingFunctors_; // loop是否有需要执行的回调
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的回调操作
    std::mutex mutex_; //保证线程安全操作
};
