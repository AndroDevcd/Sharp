//
// Created by bnunnally on 8/17/21.
//

#include "task_controller.h"
#include "scheduler.h"
#include "idle_scheduler.h"
#include "thread_controller.h"
#include "../Thread.h"
#include "../VirtualMachine.h"

fiber* create_task(string &name, Method *main) {
    fiber *fib = (fiber*)__malloc(sizeof(fiber));

    try {
        fib = new (fib)fiber();
        fib->name = name;
        fib->main = main;
        fib->pc = 0;
        fib->cache = 0;
        fib->state = FIB_CREATED;
        fib->exitVal = 0;
        fib->delayTime = -1;
        fib->wakeable = true;
        fib->finished = false;
        fib->attachedThread = NULL;
        fib->boundThread = NULL;
        fib->exceptionObject.object = NULL;
        fib->fiberObject.object = NULL;
        fib->dataStack = NULL;
        fib->registers = NULL;
        fib->callStack = NULL;
        fib->acquiringMut = NULL;
        fib->calls = -1;
        fib->current = NULL;
        fib->ptr = NULL;
        fib->stackLimit = internalStackSize;
        fib->registers = (double *) __calloc(REGISTER_SIZE, sizeof(double));
        new (&fib->mut) std::recursive_mutex();

        if(internalStackSize < INITIAL_STACK_SIZE) {
            fib->stackSize = internalStackSize;
        }
        else if((vm.manifest.threadLocals + 50) < INITIAL_STACK_SIZE) {
            fib->stackSize = INITIAL_STACK_SIZE;
        }
        else {
            fib->stackSize = vm.manifest.threadLocals + INITIAL_STACK_SIZE;
        }

        fib->dataStack = (StackElement *) __malloc(fib->stackSize* sizeof(StackElement));
        StackElement *ptr = fib->dataStack;
        for(Int i = 0; i < fib->stackSize; i++) {
            ptr->object.object = NULL;
            ptr->var=0;
            ptr++;
        }

        if(internalStackSize - vm.manifest.threadLocals < INITIAL_FRAME_SIZE) {
            fib->frameSize = internalStackSize - vm.manifest.threadLocals;
            fib->callStack = (Frame *) __malloc(fib->frameSize * sizeof(Frame));
        }
        else {
            fib->frameSize = INITIAL_FRAME_SIZE;
            fib->callStack = (Frame *) __malloc(fib->frameSize * sizeof(Frame));
        }

        fib->frameLimit = internalStackSize - vm.manifest.threadLocals;
        fib->fp = &fib->dataStack[vm.manifest.threadLocals];
        fib->sp = (&fib->dataStack[vm.manifest.threadLocals]) - 1;

        gc.addMemory(sizeof(Frame) * fib->frameSize);
        gc.addMemory(sizeof(StackElement) * fib->stackSize);
        gc.addMemory(sizeof(double) * REGISTER_SIZE);

        post(fib);
    } catch(Exception &e) {
        fib->free();
        std::free(fib);
        throw;
    }

    return fib;
}

Int bound_task_count(Thread* thread) {
    if(thread != NULL) {
        GUARD(thread->mutex)
        return thread->boundFibers;
    }

    return 0;
}

bool start_task(Thread* thread, fiber* task) {
    GUARD(task->mut)
    if(task->state == FIB_SUSPENDED && task->attachedThread == NULL
        && (task->boundThread == NULL || task->boundThread == thread)) {
        task->attachedThread = (thread);
        set_task_state(thread, task, FIB_RUNNING, NO_DELAY);
        return true;
    } else return false;
}

void delay_task(uInt time, bool incPc) {
    if(time < 0)
        time = NO_DELAY;

    fiber *fib = thread_self->this_fiber;
    Thread *thread = thread_self;
    if(try_context_switch(thread, incPc)) {
        enable_context_switch(thread, true);
        if (fib->state != FIB_KILLED)
            set_task_state(thread, fib, FIB_SUSPENDED, time);
    } else  {
        __usleep(time * 1000); // will become a sleep() call if called from c++ env
    }
}

void set_attached_thread(fiber* task, Thread* thread) {
    GUARD(task->mut)
    task->attachedThread = thread;
}

Thread* get_bound_thread(fiber* task) {
    GUARD(task->mut)
    return task->boundThread;
}

Thread* get_attached_thread(fiber* task) {
    GUARD(task->mut)
    return task->attachedThread;
}

Int get_state(fiber* task) {
    GUARD(task->mut)
    auto result = (Int)task->state;

    return result;
}

bool is_bound(fiber *fib, Thread *thread) {
    return fib->boundThread == thread && fib->state != FIB_KILLED;
}

Int bind_task(fiber* task, Thread* thread) {
    GUARD(task->mut)

    if(thread != NULL) {
        std::lock_guard<recursive_mutex> guard2(thread->mutex);
        if(thread->state != THREAD_KILLED || !hasSignal(thread->signal, tsig_kill)) {
            task->boundThread = thread;
            thread->boundFibers++;
            return 0;
        }
    } else {
        if(task->boundThread) {
            std::lock_guard<recursive_mutex> guard2(task->boundThread->mutex);
            task->boundThread->boundFibers--;
        }

        task->boundThread = NULL;
        return 0;
    }

    return 1;
}

void set_task_state(Thread* thread, fiber* task, fiber_state newState, uInt delay) {
    GUARD(task->mut)

    switch(newState) {
        case FIB_CREATED:
            task->state = newState;
            task->delayTime = -1;
            break;
        case FIB_RUNNING:
            thread->lastRanMicros = NANO_TOMICRO(Clock::realTimeInNSecs());
            task->state = newState;
            task->delayTime = -1;
            break;
        case FIB_SUSPENDED: {

            if(delay > 0) {
                task->delayTime = NANO_TOMILL(Clock::realTimeInNSecs()) + delay;
            } else task->delayTime = -1;
            task->state = newState;
            break;
        }
        case FIB_KILLED:
            bind_task(task, NULL);
            task->state = newState;
            task->finished = true;
            task->delayTime = -1;
            break;
    }
}

void set_task_wakeable(fiber* task, bool enable) {
    GUARD(task->mut)
    task->wakeable = enable;
}

Int unsuspend_task(uInt id) {
    fiber *fib = locate_task(id);
    int result = 0;

    if(fib) {
        GUARD(fib->mut)
        if(fib->state == FIB_SUSPENDED) {
            set_task_wakeable(fib, true);
        } else {
            result = 1;
        }

    }

    return result;
}

Int suspend_task(uInt id) {
    fiber *fib = locate_task(id);
    int result = 0;

    if(fib) {
        GUARD(fib->mut)
        if(fib->state == FIB_SUSPENDED) {
            set_task_wakeable(fib, false);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                set_task_wakeable(fib, false);
                set_task_state(fib->attachedThread, fib, FIB_SUSPENDED, NO_DELAY);
                enable_context_switch(fib->attachedThread, true);
            } else {
                result = 2;
            }
        } else {
            result = 1;
        }

    }

    return result;
}

fiber *locate_task(uInt taskId) {
    GUARD(task_mutex)
    sched_task *task = sched_tasks;

    while(task != NULL) {
        if(task->task->id == taskId)
            return task->task;

        task = task->next;
    }

    return locate_idle_task(taskId);
}

int kill_task(uInt taskId) {
    fiber *task = locate_task(taskId);
    if(task != NULL) return kill_task(task);
    else return 0;
}

int kill_task(fiber *fib) {
    int result = 1;

    if(fib) {
        if(fib->state == FIB_SUSPENDED) {
            set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
        } else if(fib->state == FIB_RUNNING) {
            if(fib->attachedThread) {
                set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
                enable_context_switch(fib->attachedThread, true);
            } else {
                result = 0;
            }
        } else {
            set_task_state(NULL, fib, FIB_KILLED, NO_DELAY);
        }

    }

    return result;
}

void kill_bound_fibers(recursive_mutex &mut, sched_task *starting_task, Thread *thread) {
    GUARD(mut)
    sched_task *task = starting_task;

    while (task != NULL) {
        if (is_bound(task->task, thread)) {
            kill_task(task->task);
        }

        task = task->next;
    }
}

void kill_bound_fibers(Thread* thread) {
    kill_bound_fibers(task_mutex, sched_tasks, thread);
    kill_bound_fibers(idle_mutex, idle_tasks, thread);
}
