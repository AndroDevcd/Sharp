//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../stdimports.h"

#define LPTSI 50  /* low priority time slice interval */  // ~20k fibers/thread
#define NPTSI 100  /* norm priority time slice interval */ // ~10k fibers/thread
#define HPTSI 150 /* high priority time slice interval */ // ~6k fibers/thread
#define CSTL 15    /* Context switch time limit */
#define CSST 2    /* Context switch sleep time */

extern atomic<bool> threadReleaseBlock;
void run_scheduler();
bool try_context_switch(Thread *thread, fiber *fib);
bool is_thread_ready(Thread *thread);
void __usleep(unsigned int usec);

#endif //SHARP_SCHEDULER_H
