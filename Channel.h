#pragma once

#include "noncopyable.h"

#include <memory>
#include <functional>

class EventLoop;
class Timestamp;
/*
 * 封装fd以及感兴趣的event
 * 并且绑定poller返回的及具体事件
 */
class Channel : noncopyable ,std::enable_shared_from_this<Channel>{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);

    // 得到poller通知以后处理事件
    void handleEvent(Timestamp recieveTime);

    // 设置回调函数
    void setReadCallback(ReadEventCallback cb){readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb);}

    // 防止Channel在执行操作过程中被释放
    //void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revent){revents_ = revent;}

    // 设置感兴趣的操作
    void enableReading(){events_ |= kReadEvent; update();}
    void disableReading(){events_ &= ~kReadEvent; update();}
    void enableWriting(){events_ |= kWriteEvent; update();}
    void disableWriting(){events_ &= ~kWriteEvent; update();}
    void disableAll(){events_ = kNoneEvent; update();}

    // 返回fd的状态
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isReading() const {return events_ & kReadEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}

    int status() {return status_;}
    void set_status(int idx){status_ = idx;}
    // 绑定TcpConnection
    void tie(const std::shared_ptr<void> &obj);

    EventLoop* ownerLoop(){return loop_;}
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp recieveTime);

    static const int kNoneEvent;     
    static const int kReadEvent;   
    static const int kWriteEvent;   
    
    EventLoop* loop_; // 事件循环
    const int fd_; // fd, poller监听的对象
    int events_; // 注册fd感兴趣的事件
    int revents_; // poller接收到的事件
    int status_; //在poller中的状态  
    int tied_;
    std::weak_ptr<void> tie_;

    // fd发生事件的回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_; 
    EventCallback errorCallback_;
};