//
// System interrupts for operating-system threads.
//
// Moved verbatim from the old switch in virtual_machine.cpp: the OP_THREAD_*
// family, plus CORES and YIELD. (Fibers - the VM's own lightweight tasks - are
// separate, in fibers.cpp.) See interrupt_internal.h for what a handler is.
//
#include "interrupt_internal.h"
#include <thread>

//
// thread_start(id, stackSize): hand a prepared Thread object to a real OS
// thread and start it. ADX holds the thread id, EBX the stack size. Before
// starting, the fields the thread needs (its Thread object, its args, priority,
// stack size, name) are copied across. The start result goes in CMT.
//
void op_thread_start(Int interrupt) {
    auto task = thread_self->task;
    auto thread = get_thread((int32_t )registers[ADX].i);

    if(thread != NULL) {
        copy_object(&thread->currentThread, pop_stack_object.o);
        copy_object(&thread->args, pop_stack_object.o);

        auto priorityEnum = resolve_field("priority", thread->currentThread.o);
        if(priorityEnum != NULL && priorityEnum->o != NULL) {
            thread->priority = (int) number_field(priorityEnum->o, "ordinal");
        }

        thread->stackSize = (int) number_field(thread->currentThread.o, "stack_size");

        auto name = resolve_field("name", thread->currentThread.o);
        if(name != NULL) {
            auto data = resolve_field("data", name->o);

            if(data) {
                string threadName = read_string_value(data->o);
                if(threadName != "") {
                    thread->name = threadName;
                }
            }
        }
    }
    registers[CMT].i=start_thread((int32_t )registers[ADX].i, (size_t )registers[EBX].i);
}

// thread_join(id): wait for a thread to finish. Result in CMT.
void op_thread_join(Int interrupt) {
    registers[CMT].i=join_thread((int32_t )registers[ADX].i);
}

// thread_interrupt(id): ask a thread to stop what it is waiting on. Result in CMT.
void op_thread_interrupt(Int interrupt) {
    registers[CMT].i=interrupt_thread((int32_t )registers[ADX].i);
}

// thread_create(id, daemon): make a new OS thread record. New id in EBX.
void op_thread_create(Int interrupt) {
    registers[EBX].i=create_thread((int32_t )registers[ADX].i, (bool)registers[EBX].i);
}

// thread_priority(id, priority): set a thread's scheduling priority. Result in CMT.
void op_thread_priority(Int interrupt) {
    registers[CMT].i=set_thread_priority((int32_t )registers[ADX].i, (int)registers[EGX].i);
}

// thread_suspend(id): pause a thread. Result in CMT.
void op_thread_suspend(Int interrupt) {
    registers[CMT].i=suspend_thread((int32_t )registers[ADX].i);
}

// thread_unsuspend(id, ...): resume a thread. Result in CMT.
void op_thread_unsuspend(Int interrupt) {
    registers[CMT].i=unsuspend_thread((int32_t )registers[ADX].i, (int32_t )registers[EBX].i);
}

// thread_current(): push this thread's Thread object. (native getCurrentThread())
void op_thread_current(Int interrupt) {
    auto task = thread_self->task;
    grow_stack
    stack_overflow_check
    copy_object(&push_stack_object, thread_self->currentThread.o);
}

// thread_args(): push this thread's start arguments. (native getCurrentThreadArgs())
void op_thread_args(Int interrupt) {
    auto task = thread_self->task;
    grow_stack
    stack_overflow_check
    copy_object(&push_stack_object, thread_self->args.o);
}

// thread_set_current(Thread): set this thread's Thread object from the stack.
void op_thread_set_current(Int interrupt) {
    auto task = thread_self->task;
    copy_object(&thread_self->currentThread, pop_stack_object.o);
}

// cores(): how many hardware threads the machine has, into EBX.
void op_cores(Int interrupt) {
    registers[EBX].i=std::thread::hardware_concurrency();
}

// yield(): give up the rest of this time slice to another thread.
void op_yield(Int interrupt) {
    std::this_thread::yield();
}
