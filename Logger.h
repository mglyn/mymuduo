#pragma once

#include <string>
#include <iostream>

#include "noncopyable.h"
#include "Timestamp.h"

#define MUDEBUG

#define LOG_INFO(logmsgFormat, ...) \
    do{ \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do{ \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do{ \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    }while(0)

#ifdef MUDEBUG
    #define LOG_DEBUG(logmsgFormat, ...) \
    do{ \
        Logger& logger = Logger::GetInstance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

//日志级别
enum LogLevel{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class Logger : noncopyable{
public:
    static Logger& GetInstance(){
        static Logger logger;
        return logger;
    }
    void setLogLevel(int level){
        logLevel_ = level;
    }
    void log(std::string msg){
        if(logLevel_ == INFO)
            std::cout << "[INFO]";
        else if(logLevel_ == ERROR)
            std::cout << "[ERROR]";
        else if(logLevel_ == FATAL)
            std::cout << "[FATAL]";
        else if(logLevel_ == DEBUG)
            std::cout << "[DEBUG]";        
        
        std::cout << Timestamp::now().toString() << " : ";

        std::cout << msg << std::endl;
    }
private:
    int logLevel_;
    Logger(){}
};