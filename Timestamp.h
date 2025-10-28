#pragma once

#include <string>
#include <sys/time.h>
#include <time.h>

class Timestamp{
public:
    Timestamp() :microSecondsSinceEpoch_(0){};

    explicit Timestamp(int64_t microSecondsSinceEpoch) :microSecondsSinceEpoch_(microSecondsSinceEpoch){}

    static Timestamp now(){
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t seconds = tv.tv_sec;
        return Timestamp(seconds * 1000000ULL+ tv.tv_usec);
    }

    std::string toString(bool showMicroSeconds = false) const{
        char buf[64] = {0};
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / 1000000ULL);
        struct tm tm_time;
        localtime_r(&seconds, &tm_time);

        if (showMicroSeconds){
            int microseconds = static_cast<int>(microSecondsSinceEpoch_ % 1000000ULL);
            snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                    tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                    tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                    microseconds);
        }
        else{
            snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                    tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                    tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }
        return buf;
    }

private:
    int64_t microSecondsSinceEpoch_;
};

