#pragma once

#include <vector>
#include <string>
#include <algorithm>

class Buffer{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t inintalSize = kInitialSize):
        buffer_(kCheapPrepend + inintalSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend){}

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const {
        return readerIndex_;
    }

    // 可读区起始地址
    const char* peek() const {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len){
        if(len < readableBytes()){
            readerIndex_ += len;
        }
        else{
            retrieveAll();
        }
    }

    void retrieveAll(){
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    //onMessage buffer -> string
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);
        }
    }

    void append(const char* data, size_t len){
        ensureWritableBytes(len);
        std::copy(data, data + len, begin() + writerIndex_);
        writerIndex_ += len;
    } 

    //从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);

private:

    char* begin(){
        return &*buffer_.begin();
    }
    const char* begin() const {
        return &*buffer_.begin();
    }
    void makeSpace(size_t len){
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){
            // 可写加空余不足直接把可写区扩容至len
            buffer_.resize(writerIndex_ + len);
        }
        else{
            // 虽然可写区空间不足 但前方已经有空余 相加空间足够 挪动凑够空间
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};