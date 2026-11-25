//
// System interrupts for fibers - the VM's own lightweight tasks (coroutines),
// which many of them share one OS thread. (Real OS threads are in threads.cpp.)
//
// Moved verbatim from the old switch in virtual_machine.cpp (OP_FIBER_*).
// See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"

//
// fiber_start(args, fiber, threadId, mainFunc, name): create a fiber and hand it
// to the scheduler. The arguments are popped top-first: name, the main function
// (a function value), the thread id to bind to (-1 = none), the Fiber object,
// and its arguments. The new fiber's id goes in EBX.
//
void op_fiber_start(Int interrupt) {
    auto task = thread_self->task;

    auto *name = pop_stack_object.o;
    // the function the fiber runs is a function value (an object);
    // only its index is needed, a fiber's main captures nothing
    Int mainFunc = function_value_address(pop_stack_object.o);
    Int threadid = pop_stack_number.i;
    auto fibObj = pop_stack_object.o;
    auto fibArgs = pop_stack_object.o;

    if(name != NULL && name->type <= type_var) {
        string fiberName;
        populate_string(fiberName, name);
        fiber *fib;
        sharp_thread *binder = nullptr;

        if(threadid != -1) {
            if(threadid != gc_threadid && threadid != idle_threadid) {
                binder = get_thread(threadid);
            } else {
                registers[EBX].i = -1;
                return;
            }
        }

        // create_task() hands the fiber to the scheduler. From that
        // moment another thread may run it to its end and free it, so
        // `fib` must not be used after the scheduler lock is let go:
        // the id is read while the lock still keeps the fiber alive
        // (freeing a fiber, sched_reap, needs the same lock). Reading
        // fib->id afterwards was a use after free - the rare crash in
        // start_fiber.
        Int fiberId;
        try {
            std::lock_guard<recursive_mutex> keepFiberAlive(task_mutex);
            fib = create_task(fiberName, &vm.methods[mainFunc % vm.mf.methods], fibObj, fibArgs, binder);
            fiberId = fib->id;
        } catch(vm_exception &e) {
            fiberName.clear();
            throw;
        }

        registers[EBX].i = fiberId;
    } else {
        throw vm_exception(vm.nullptr_except, "");
    }
}

// fiber_suspend(id): pause a fiber. Result in EBX.
void op_fiber_suspend(Int interrupt) {
    auto task = thread_self->task;
    Int fiberId = pop_stack_number.i;
    registers[EBX].i = suspend_task(fiberId);
}

// fiber_unsuspend(id): resume a fiber. Result in EBX.
void op_fiber_unsuspend(Int interrupt) {
    auto task = thread_self->task;
    Int fiberId = pop_stack_number.i;
    registers[EBX].i = unsuspend_task(fiberId);
}

// fiber_kill(id): end a fiber. Result in EBX.
void op_fiber_kill(Int interrupt) {
    auto task = thread_self->task;
    Int fiberId = pop_stack_number.i;
    registers[EBX].i = kill_task(fiberId);
}

//
// fiber_bind(threadId, fiberId): pin a fiber to a particular OS thread. The
// thread is looked up BEFORE taking task_mutex (thread_mutex must never be taken
// inside it). Result in EBX (2 = no such fiber).
//
void op_fiber_bind(Int interrupt) {
    auto task = thread_self->task;
    Int fiberId = pop_stack_number.i;
    Int threadId = pop_stack_number.i;
    auto bindTo = get_thread(threadId); // before task_mutex: thread_mutex must never be taken inside it

    guard_mutex(task_mutex) // the fiber cannot be freed while we use it
    fiber *fib = locate_task(fiberId);
    registers[EBX].i = 2;

    if(fib) {
        registers[EBX].i = bind_task(fib, bindTo);
    }
}

// fiber_bound_count(threadId): how many fibers are pinned to a thread, into EBX.
void op_fiber_bound_count(Int interrupt) {
    auto task = thread_self->task;
    Int threadId = pop_stack_number.i;
    auto thread = get_thread(threadId);
    registers[EBX].i = thread ? thread->boundFibers.load() : 0;
}

// fiber_state(id): a fiber's current state, into EBX (-1 = no such fiber).
void op_fiber_state(Int interrupt) {
    auto task = thread_self->task;
    Int fiberId = pop_stack_number.i;
    guard_mutex(task_mutex) // the fiber cannot be freed while we use it
    fiber *fib = locate_task(fiberId);
    registers[EBX].i = fib ? get_state(fib) : -1;
}

// fiber_current(): push the running fiber's Fiber object.
void op_fiber_current(Int interrupt) {
    auto task = thread_self->task;
    grow_stack
    stack_overflow_check
    copy_object(&push_stack_object, task->fiberObject.o);
}
