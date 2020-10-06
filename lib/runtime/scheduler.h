//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../stdimports.h"

#define LPTSI 500  /* low priority time slice interval 500us */  // ~2k fibers/thread
#define NPTSI 1000  /* norm priority time slice interval 1ms */ // ~1k fibers/thread
#define HPTSI 1500 /* high priority time slice interval 1.5ms */ // ~666 fibers/thread
#define CLOCK_CYCLE 50    /* Time between clock cycle switches 550us */

extern uInt clocks;
extern atomic<bool> threadReleaseBlock;
void run_scheduler();
bool is_thread_ready(Thread *thread);
void __usleep(unsigned int usec);

#endif //SHARP_SCHEDULER_H
