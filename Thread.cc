#include "Thread.h"
#include "CurrentThread.h"

#include <condition_variable>

std::atomic_int Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string& name):
    started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name)
{
    setDefaultName();
}

Thread::~Thread(){
    if(started_ && !joined_){
        thread_->detach();        
    }
}

void Thread::start(){
    started_ = true;
    
    std::unique_lock<std::mutex> lock(mutex_);

    thread_ = std::shared_ptr<std::thread> (new std::thread([this](){
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tid_ = CurrentThread::tid();
            cond_.notify_one();
        }
        func_();
    }));
    
    cond_.wait(lock, [this](){return tid_;});
}

void Thread::join(){
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num = ++numCreated_;
    if(name_.empty()){
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf; 
    }
}