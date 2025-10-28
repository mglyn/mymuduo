#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallBack& cb, const std::string& name):
    loop_(nullptr),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    callback_(cb)
    {}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop(){
    std::unique_lock<std::mutex> lock(mutex_);
    thread_.start();
    cond_.wait(lock, [this](){return loop_;});
    return loop_;
}

void EventLoopThread::threadFunc(){
    EventLoop loop; 

    if(callback_){
        callback_(&loop);
    }
    { 
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}