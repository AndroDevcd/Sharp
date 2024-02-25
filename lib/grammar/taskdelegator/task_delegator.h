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
            thread(0)
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
                thread(0)
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
        thread(0)
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

#endif //SHARP_TASK_DELEGATOR_H
