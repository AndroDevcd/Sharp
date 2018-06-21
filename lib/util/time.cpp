//
// Created by BraxtonN on 1/22/2018.
//

#include "time.h"
#include <time.h>
#include <chrono>
#include "../../stdimports.h"

struct tm  tstruct;
int Clock::__os_time(int ty) {
    time_t     now = time(0);
    tstruct = *localtime(&now);
    if(ty==0)
        return tstruct.tm_sec;
    else if(ty==1)
        return tstruct.tm_min;
    else if(ty==2)
        return tstruct.tm_hour;
    else if(ty==3)
        return tstruct.tm_mday;
    else if(ty==4)
        return tstruct.tm_mon;
    else if(ty==5)
        return tstruct.tm_year;
    else if(ty==6)
        return tstruct.tm_wday;
    else if(ty==7)
        return tstruct.tm_yday;
    else if(ty==8)
        return tstruct.tm_isdst;
    else
        return tstruct.tm_sec;
}

/*
* The total ammount of time in nano seconds
* since Jan 1, 1970 (UTC Format)
*/
int64_t Clock::realTimeInNSecs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>
            (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}