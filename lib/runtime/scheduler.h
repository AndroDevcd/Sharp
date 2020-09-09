//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#define LPTSI 25  /* low priority time slice interval */  // 40 fibers/thread
#define NPTSI 70  /* norm priority time slice interval */ // 14 fibers/thread
#define HPTSI 120 /* high priority time slice interval */ // 8 fibers/thread
#define CSTL 5    /* Context switch time limit */

void run_scheduler();
bool try_context_switch(fiber *fib);

#endif //SHARP_SCHEDULER_H
