#include "EventLoop.h"
#include "Logger.h"
#include "EpollPoller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <errno.h>
#include <memory>

thread_local EventLoop *t_loopInThisThread = nullptr; // 保证每个线程只有一个loop

const int kPollTimeMiliSecond = 10000; // poller 超时时间

// 创建用来通知subReactor的eventfd
int creatEventFd(){
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_FATAL("eventfd error: %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop(): 
    looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(new EpollPoller(this)),
    wakeupFd_(creatEventFd()),
    wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop Created %p in thread %d\n", this, threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("the other EventLoop %p has already been created on this thread %d", t_loopInThisThread, threadId_);
    }
    else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件及回调
    auto handleRead = [this](){
        uint64_t one = 1;
        ssize_t n = read(wakeupFd_, &one, sizeof one);
        if(n != sizeof one){
            LOG_ERROR("EventLoop handleRead reads %lu bytes instead of 8", n);
        }
    };

    wakeupChannel_->setReadCallback(std::bind(handleRead));
    wakeupChannel_->enableReading(); 
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop(){
    looping_ =  true;
    quit_ = false;

    LOG_INFO("EventLoop: %p start looping\n", this);

    while(!quit_){
        activeChannels_.clear();

        pollReturnTime_ = poller_->poll(kPollTimeMiliSecond, &activeChannels_);

        for(Channel* channel : activeChannels_){
            // 处理poller监听到的事件
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors(); 
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
}

void EventLoop::quit(){
    quit_ = true;
    if(!isInLoopThread()){ //若是其他线程的loop 将其唤醒退出
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();               
    }
    else{
         queueInLoop(cb);   //若是其他线程的EventLoop, 写入待执行        
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup(); //若loop在其他线程，提醒loop所在线程处理 若loop在自身线程正在处理回调，向未来的自身线程发出新的待处理回调提醒
    }    
}

//写一个数据用于唤醒loop所在线程
void EventLoop::wakeup(){ 
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex>(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const auto& functor : functors){
        functor();
    }
    callingPendingFunctors_ = false;
}

