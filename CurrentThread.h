#pragma once

namespace CurrentThread{

    extern thread_local int t_cachedTid;

    void cacheTid();

    inline int tid(){
        [[unlikely]] if(t_cachedTid == 0){
            cacheTid();
        }
        return t_cachedTid;
    }
}