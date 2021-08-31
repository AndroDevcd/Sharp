//
// Created by bnunnally on 8/30/21.
//
#include "task_delegator.h"

thread_local worker_thread *currThread;

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
workerStart(void *arg) {
    currThread = (worker_thread*)arg;
    currThread->state = worker_idle;

    do {
        if(currThread->nextTask != NULL) {
            currThread->currTask = currThread->nextTask;
            currThread->nextTask = NULL;
        }

        if(currThread->currTask != NULL) {
            execute_task();
            currThread->currTask = NULL;
            currThread->state = worker_idle;
        }

        __os_yield();
#ifdef WIN32_
        Sleep(25);
#endif
#ifdef POSIX_
        usleep(25*POSIX_USEC_INTERVAL);
#endif
    } while(true);

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void execute_task() {
    cout << "executing task(" << currThread->currTask->type << ") on: " << currThread->currTask->file->name << endl;
}