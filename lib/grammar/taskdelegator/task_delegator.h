//
// Created by BNunnally on 8/30/2021.
//

#ifndef SHARP_TASK_DELEGATOR_H
#define SHARP_TASK_DELEGATOR_H

#include "task.h"

enum worker_state {
    none,
    worker_running,
    worker_idle,
    worker_dead
};

struct worker_thread {

    worker_thread()
        :
            id(-1),
            state(none),
            currTask(NULL),
            nextTask(NULL),
            thread(NULL)
    {}


    worker_thread(
            int id,
            worker_state state,
            task *pTask)
            :
                id(id),
                state(state),
                currTask(pTask),
                nextTask(NULL),
                thread(NULL)
    {}

    Int id;
    worker_state state;
    task *currTask;
    task *nextTask;

#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif
};

struct task_delegator {
    task_delegator()
    :
        alive(false),
        allWorkersFree(false),
        thread(NULL)
    {}

    bool alive;
    bool allWorkersFree;
#ifdef WIN32_
    HANDLE thread;
#endif
#ifdef POSIX_
    pthread_t thread;
#endif
};

extern recursive_mutex errorMutex;
extern task_delegator delegator;
extern thread_local worker_thread *currThread;

int start_task_delegator();
uInt start_worker(worker_thread*);
worker_thread* create_worker();
void killAllWorkers();

int wait_for_idle(worker_thread*);

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
delegatorStart(void *);

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
workerStart(void *);

void wait_for_tasks();
void submit_task(task t);
bool post_task(task);
void execute_task();
void remove_all_posted_tasks(sharp_file *file);


enum ThreadProcessingResult {
    RESULT_OK                      = 0x000,
    RESULT_ILL_THREAD_START        = 0x001,
    RESULT_THREAD_RUNNING          = 0x002,
    RESULT_THREAD_TERMINATED       = 0x003,
    RESULT_INVALID_STACK_SIZE      = 0x004,
    RESULT_THREAD_NOT_STARTED      = 0x005,
    RESULT_ILL_THREAD_JOIN         = 0x006,
    RESULT_THREAD_JOIN_FAILED      = 0x007,
    RESULT_ILL_THREAD_INTERRUPT    = 0x008,
    RESULT_THREAD_INTERRUPT_FAILED = 0x009,
    RESULT_ILL_THREAD_DESTROY      = 0x00a,
    RESULT_THREAD_CREATE_FAILED    = 0x00b,
    RESULT_NO_THREAD_ID            = 0x00c,
    RESULT_ILL_PRIORITY_SET        = 0x00e,
    RESULT_ILL_THREAD_SUSPEND      = 0x00f,
    RESULT_MAX_SPIN_GIVEUP         = 0x0ff
};

#endif //SHARP_TASK_DELEGATOR_H
