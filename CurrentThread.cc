#include "CurrentThread.h"

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread{

    thread_local int t_cachedTid = 0;

    void cacheTid(){
        if(t_cachedTid == 0){
            //linux 系统调用获得当前线程tid
            t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
        }
    }

}