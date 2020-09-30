//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../stdimports.h"

#define LPTSI 100  /* low priority time slice interval 100us */  // ~10k fibers/thread
#define NPTSI 150  /* norm priority time slice interval 150us */ // ~6k fibers/thread
#define HPTSI 250 /* high priority time slice interval 250us */ // ~4k fibers/thread
#define CLOCK_CYCLE 55    /* Time between clock cycle switches 55us */

extern atomic<bool> threadReleaseBlock;
void run_scheduler();
bool is_thread_ready(Thread *thread);
void __usleep(unsigned int usec);

#endif //SHARP_SCHEDULER_H
