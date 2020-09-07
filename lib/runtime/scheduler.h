//
// Created by BNunnally on 9/5/2020.
//

#ifndef SHARP_SCHEDULER_H
#define SHARP_SCHEDULER_H

#define LPTSI 25  /* low priority time slice interval */
#define NPTSI 70  /* norm priority time slice interval */
#define HPTSI 120 /* high priority time slice interval */
#define CSTL 5    /* Context switch time limit */

void run_scheduler();
bool try_context_switch(fiber *fib);

#endif //SHARP_SCHEDULER_H
