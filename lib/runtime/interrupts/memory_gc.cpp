//
// System interrupts for the garbage collector and for raw memory.
//
// Moved verbatim from the old switch in virtual_machine.cpp: the OP_GC_* family,
// the memory counters, and the block operations (copy, invert, realloc,
// serialize). See interrupt_internal.h for what a handler is.
//

#include "interrupt_internal.h"

//
// gc_explicit(): ask the collector to run. It is only a request - it runs at the
// next safe point, right after this instruction, and may decline.
//
void op_gc_explicit(Int interrupt) {
    tgc_request(true);
}

// gc_sleep(): stop the collector from running until woken.
void op_gc_sleep(Int interrupt) {
    sedate_gc();
}

// gc_wakeup(): let the collector run again.
void op_gc_wakeup(Int interrupt) {
    wake_gc();
}

// gc_kill(): shut the collector thread down (used while the VM is ending).
void op_gc_kill(Int interrupt) {
    kill_gc();
}

// gc_state(): the collector's current state, into CMT.
void op_gc_state(Int interrupt) {
    registers[CMT].i = gc.state;
}

// memory_limit(): the ceiling on managed memory, into CMT.
void op_memory_limit(Int interrupt) {
    registers[CMT].i = gc.memoryLimit;
}

// memory(): how many managed bytes are in use now, into CMT.
void op_memory(Int interrupt) {
    registers[CMT].i = gc.managedBytes;
}

// sizeof(object): the size of an object (its slot count), into EBX.
void op_sizeof(Int interrupt) {
    auto task = thread_self->task;
    registers[EBX].i = sizeof_object(pop_stack_object.o);
}

// mem_copy(...): copy a run of elements between two arrays (helper does the work).
void op_mem_copy(Int interrupt) {
    memcopy();
}

// invert(...): reverse a run of elements in place (helper does the work).
void op_invert(Int interrupt) {
    invert();
}

//
// realloc(array, length): grow or shrink an array to `length` elements. The
// length is popped; the array is read from the top of the stack in place.
//
void op_realloc(Int interrupt) {
    auto task = thread_self->task;

    size_t len = pop_stack_number.i;
    auto array = task->sp->obj.o;

    if(array != NULL) {
        realloc_object(array, len);
    } else
        throw vm_exception(vm.nullptr_except, "");
}

// copy(...): shallow-copy an object (helper does the work).
void op_copy(Int interrupt) {
    copy();
}

//
// serialize(object): turn an object graph into bytes, in place on the stack.
//
void op_serialize(Int interrupt) {
    auto task = thread_self->task;
    serialize(&task->sp->obj, &task->sp->obj);
}

//
// deserialize(bytes): rebuild an object graph from bytes. Reads the bytes below
// the top slot and leaves the rebuilt object on the stack.
//
void op_deserialize(Int interrupt) {
    auto task = thread_self->task;
    deserialize(&task->sp->obj, &(task->sp+1)->obj);
    copy_object(&task->sp->obj, &(task->sp+1)->obj);
}
