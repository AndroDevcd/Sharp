//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#include "../../stdimports.h"

#define LPTSI 5  /* low priority time slice interval */  // 200 fibers/thread
#define NPTSI 10  /* norm priority time slice interval */ // 100 fibers/thread
#define HPTSI 20 /* high priority time slice interval */ // 50 fibers/thread
#define CSTL 3    /* Context switch time limit */

extern atomic<bool> threadReleaseBlock;
void run_scheduler();
bool try_context_switch(Thread *thread, fiber *fib);
bool is_thread_ready(Thread *thread);

#endif //SHARP_SCHEDULER_H
