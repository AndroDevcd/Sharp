//
// Created by BNunnally on 8/30/2021.
//

#include "task_delegator.h"
#include "../settings/settings.h"
#include "../../core/thread_state.h"

Int maxWorkers = 0;
List<worker_thread*> workers;
List<task> task_queue;
recursive_mutex taskMutex;
recursive_mutex errorMutex;
task_delegator delegator;
Int workerId = 0;

int start_task_delegator() {
#ifdef WIN32_
    delegator.thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            (LPTHREAD_START_ROUTINE)delegatorStart,       // thread function caller
            &delegator,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(delegator.thread == NULL) return RESULT_THREAD_NOT_STARTED; // thread was not started
#endif
#ifdef POSIX_
    if(pthread_create( &delegator.thread, NULL, delegatorStart, (void*) thread)!=0)
        return RESULT_THREAD_NOT_STARTED; // thread was not started
#endif

    return RESULT_OK;
}

void calculateMaxWorkers() {
    maxWorkers = std::thread::hardware_concurrency() * 2;
    maxWorkers--; // for delegator thread
}

void throttle_max_threads() {
    if(maxWorkers >= 3)
        maxWorkers /= 3;
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
delegatorStart(void *) {
//    calculateMaxWorkers();

    delegator.alive = true;
    if(options.green_mode) {
        throttle_max_threads();
    }

    if(maxWorkers <= 0)
        maxWorkers = 1;

    for(Int i = 0; i < maxWorkers; i++) {
        start_worker(create_worker());
    }

    do {
        if(!panic && !task_queue.empty()) {
            GUARD(taskMutex)

            if(!delegator.alive)
                break;

            for(Int i = 0; i < task_queue.size(); i++) {
                if(post_task(task_queue.get(i))) {
                    task_queue.removeAt(i); i--;
                }
            }
        }

        if(!delegator.alive)
            break;

        bool allTasksFinished = task_queue.empty();
        bool allWorkersIdle = true;
        for(Int i = 0; i < workers.size(); i++) {
            if(!(workers.get(i)->state == worker_idle
                && workers.get(i)->nextTask == NULL
                && workers.get(i)->currTask == NULL)) {
                allWorkersIdle = false;
                break;
            }
        }

        delegator.allWorkersFree = panic || (allTasksFinished && allWorkersIdle);

        __os_yield();
#ifdef WIN32_
        Sleep(10);
#endif
#ifdef POSIX_
        usleep(10*POSIX_USEC_INTERVAL);
#endif
    } while(true);

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void killAllWorkers() {
    if(delegator.allWorkersFree) {
        GUARD(taskMutex)
        delegator.alive = false;
        for(Int i = 0; i < workers.size(); i++) {
            workers.get(i)->state = worker_dead;
        }
    }
}

uInt start_worker(worker_thread* wt) {
#ifdef WIN32_
    delegator.thread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            (LPTHREAD_START_ROUTINE)workerStart,       // thread function caller
            wt,                 // thread self when thread is created
            0,                      // use default creation flags
            NULL);
    if(wt->thread == NULL) return RESULT_THREAD_NOT_STARTED; // thread was not started
    else return wait_for_idle(wt);
#endif
#ifdef POSIX_
    if(pthread_create( &wt->thread, NULL, workerStart, (void*) thread)!=0)
        return RESULT_THREAD_NOT_STARTED; // thread was not started
    else return wait_for_idle(wt);
#endif

    return RESULT_OK;
}

int wait_for_idle(worker_thread* wt) {
    const int sMaxRetries = 1000000;
    const int sMaxSpinCount = 25;

    int spinCount = 0;
    int retryCount = 0;

    while (wt->state != worker_idle)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            if(++spinCount >= sMaxSpinCount)
            {
                return RESULT_MAX_SPIN_GIVEUP;
            }

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*POSIX_USEC_INTERVAL);
#endif
        }
    }

    return RESULT_OK;
}

worker_thread* create_worker() {
    worker_thread *wt = new worker_thread();
    workers.add(wt);
    return wt;
}

void wait_for_tasks() {
    const int sMaxRetries = 1000000;

    int retryCount = 0;

    while (!delegator.allWorkersFree)
    {
        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
            __os_yield();
#ifdef WIN32_
            Sleep(50);
#endif
#ifdef POSIX_
            usleep(50*POSIX_USEC_INTERVAL);
#endif
        }
    }
}

bool post_task(task t) {
    // ensure that we only execute tasks serially one at a time for each file
    if((t.file->stage + 1) == t.type) {
        for (Int i = 0; i < workers.size(); i++) {
            worker_thread *worker = workers.get(i);

            if (worker->state == worker_idle
                && worker->nextTask == NULL
                && worker->currTask == NULL) {
                worker->nextTask = new task(t);
                return true;
            }
        }
    }

    // we can keep going after the parsed stage even if the file has failed to compile
    return t.file->compilationFailed && t.file->stage <= parsed;

}

void submit_task(task t) {
    GUARD(taskMutex)
    if(!t.file->compilationFailed) {
        task_queue.add(t);
        delegator.allWorkersFree = false;
    }
}

void remove_all_posted_tasks(sharp_file *file) {
    GUARD(taskMutex)

    for(Int i = 0; i < task_queue.size(); i++) {
        if(task_queue.get(i).file == file) {
            task_queue.removeAt(i); i--;
        }
    }
}
