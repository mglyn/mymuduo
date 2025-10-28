#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string nameArg):
    baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0){}

EventLoopThreadPool::~EventLoopThreadPool(){}

void EventLoopThreadPool::start(const ThreadInitCallback& cb){
    started_ = true;

    for(int i = 0; i < numThreads_; i++){
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        threads_.push_back(std::unique_ptr<EventLoopThread>(new EventLoopThread(cb, buf)));
        loops_.push_back(threads_[i]->startLoop());  
    } 

    if(numThreads_ == 0 && cb){
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop(){ // 轮询
    EventLoop* loop = baseLoop_;
    
    if(!loops_.empty()){
        loop = loops_[next_++];
        next_ %= loops_.size();
    }
    
    return loop;
}
