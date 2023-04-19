//
// Created by bnunnally on 8/17/21.
//

#ifndef SHARP_THREAD_CONTROLLER_H
#define SHARP_THREAD_CONTROLLER_H
#include "scheduler.h"

bool can_dispose(_sched_thread*);
void dispose(_sched_thread*);
_sched_thread* sched_thread(_sched_thread*);
void create_main_thread();
Thread* create_gc_thread();
Thread* get_main_thread();
uInt generate_thread_id();
bool valid_stack_size(uInt);
bool valid_internal_stack_size(uInt);

// thread initialization
void initialize(Thread *thread, string &name, uInt id, bool daemon, Method* main, bool initializeStack);
uInt create_thread(uInt methodAddr, bool daemon);

// thread execution manipulation
uInt suspend_thread(uInt);
uInt unsuspend_thread(uInt, bool);
uInt interrupt_thread(uInt);
uInt join_thread(uInt);

uInt join_thread(Thread*);
uInt interrupt_thread(Thread*);
void send_suspend_signal(Thread*);
void send_interrupt_signal(Thread*);
uInt send_unsuspend_signal(Thread*);
void wait_for_unsuspend_release(Thread*);
void unsuspend_and_wait(Thread*, bool);
void suspend_and_wait(Thread*, bool);
void interrupt_and_wait(Thread*);
void suspend_self();
void suspend_all_threads(bool);
void resume_all_threads(bool);

// thread starting & stopping
uInt start_thread(uInt threadId, size_t stackSize);
uInt start_thread(Thread*, size_t stackSize);

int start_daemon_thread(
#ifdef WIN32_
        DWORD WINAPI
#endif
#ifdef POSIX_
        void*
#endif
        (*threadFunc)(void*), Thread* thread);

// context switching
bool try_context_switch(Thread *, bool);
void enable_context_switch(Thread*, bool);
void wait_for_context_switch(Thread*);
void wait_for_posted_task(Thread*);
bool accept_task(Thread*);
void post_task(Thread*, fiber*);
void cancel_task(Thread*);

// priority setting
int set_thread_priority(Thread*, int);

int set_thread_priority(uInt, int);

// util thread functions
Thread* get_thread(uInt id);
uInt wait_for_thread_start(Thread*);
void wait_for_thread_exit(Thread*);
void kill_all_threads();
void shutdown_thread(Thread*);
void release_resources(Thread*);
void __os_sleep(uInt);
void setup_thread(Thread*);


#endif //SHARP_THREAD_CONTROLLER_H
