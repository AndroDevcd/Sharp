//
// Created by BraxtonN on 1/22/2018.
//

#ifndef SHARP_TIME_H
#define SHARP_TIME_H


class Clock {
public:
    static unsigned long long realTimeInNSecs();
    static int __os_time(int ty);


};

#define NANO_TOMICRO(x) ((x)/1000L)

#define NANO_TOMILL(x) (NANO_TOMICRO(x)/1000L)

#define NANO_TOSEC(x) (NANO_TOMILL(x)/1000L)

#endif //SHARP_TIME_H
