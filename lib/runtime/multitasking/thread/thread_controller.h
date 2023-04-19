//
// Created by bknun on 9/19/2022.
//

#ifndef SHARP_THREAD_CONTROLLER_H
#define SHARP_THREAD_CONTROLLER_H

#include "../scheduler/scheduler.h"
#include "../../types/sharp_function.h"

bool can_dispose(_sched_thread*);
void dispose(_sched_thread*);
_sched_thread* sched_thread(_sched_thread*);
void create_main_thread();
sharp_thread* create_gc_thread();
sharp_thread* get_main_thread();
sharp_function* get_main_method();
uInt new_thread_id();

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
vm_thread_entry(void *arg);

// thread initialization
void setup_thread(sharp_thread *thread, string &name, uInt id, bool daemon, sharp_function* main, bool initializeStack);
uInt create_thread(uInt methodAddr, bool daemon);

// thread execution manipulation
uInt suspend_thread(uInt);
uInt unsuspend_thread(uInt, bool);
uInt interrupt_thread(uInt);
uInt join_thread(uInt);

uInt join_thread(sharp_thread*);
uInt interrupt_thread(sharp_thread*);
void send_suspend_signal(sharp_thread*);
void send_interrupt_signal(sharp_thread*);
uInt send_unsuspend_signal(sharp_thread*);
void wait_for_unsuspend_release(sharp_thread*);
void unsuspend_and_wait(sharp_thread*, bool);
void suspend_and_wait(sharp_thread*, bool);
void interrupt_and_wait(sharp_thread*);
void suspend_self();
void suspend_all_threads(bool);
void resume_all_threads(bool);

// thread starting & stopping
uInt start_thread(uInt threadId, size_t stackSize);
uInt start_thread(sharp_thread*, size_t stackSize);

int start_daemon_thread(
#ifdef WIN32_
        DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
(*threadFunc)(void*), sharp_thread* thread);

// context switching
void enable_context_switch(sharp_thread*, bool);
void wait_for_posted_task(sharp_thread*);
void post_task(sharp_thread*, sched_task*);

// priority setting
int set_thread_priority(sharp_thread*, int);

int set_thread_priority(uInt, int);

// util thread functions
sharp_thread* get_thread(uInt id);
uInt wait_for_thread_start(sharp_thread*);
void wait_for_thread_exit(sharp_thread*);
void kill_all_threads();
void shutdown_thread(sharp_thread*);
void release_resources(sharp_thread*);
void __os_sleep(uInt);
void setup_thread(sharp_thread*);

#endif //SHARP_THREAD_CONTROLLER_H
