//
// Created by bknun on 4/22/2023.
//

#ifndef SHARP_IDLE_SCHEDULER_H
#define SHARP_IDLE_SCHEDULER_H

struct sharp_thread;

/*
 * The idle handler thread used to hold fibers that were asleep or blocked once
 * there were more than 1000 of them, so the scheduler did not have to scan them
 * on every pass. The scheduler no longer scans anything (waiting fibers sit in
 * the timer heap / lock wait list until an event wakes them, see scheduler.h),
 * so there is no idle pool anymore. The thread itself is kept because thread id
 * 2 (idle_threadid) is reserved for it throughout the vm.
 */
void idle_handler_startup();

#endif //SHARP_IDLE_SCHEDULER_H
