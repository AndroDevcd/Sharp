//
// Created by bknun on 9/16/2022.
//

#include "sharp_object.h"
#include "garbage_collector.h"
#include "../types/sharp_class.h"
#include "../error/vm_exception.h"
#include "memory_helpers.h"
#include "../types/sharp_field.h"
#include "../types/sharp_type.h"
#include "../multitasking/fiber/fiber.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/thread/thread_controller.h"
#include "../../core/thread_state.h"
#include "../../core/access_flag.h"
#include "../telemetry/telemetry.h"
#include "../multitasking/scheduler/scheduler.h"

std::recursive_mutex lock_mut;

void init_struct(object *o) {
    o->o = nullptr;
}

void init_struct(sharp_object *o) {
    o->next = nullptr;
    o->type = 0;
    o->size = 0;
    o->info = 0;
    o->arrayFlag = 0;
    o->HEAD = nullptr;
}

sharp_object* create_static_object(sharp_class* sc, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

    init_struct(o);
    o->size = sc->staticFields;
    o->type = type_class;
    SET_INFO(o->info, sc->address, gc_normal);

    if(o->size > 0)
    {
        o->node = calloc_mem<object>(o->size, sizeof(object), unsafe);
        for(Int i = 0; i < o->size; i++) {
            sharp_field *field = sc->fields + (sc->instanceFields + i);
            if(field->type->type <= type_var && !field->isArray && check_flag(field->flags, flag_static)) {
                copy_object(o->node + i, create_object(1, field->type->type));
            }
        }
    }

    push_object(o);
    return o;
}

sharp_object* create_object(sharp_class* sc, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

    init_struct(o);
    o->size = sc->instanceFields;
    o->type = type_class;
    SET_INFO(o->info, sc->address, gc_normal);

    if(o->size > 0)
    {
        o->node = calloc_mem<object>(o->size, sizeof(object), unsafe);
        for(Int i = 0; i < sc->instanceFields; i++) {
            sharp_field *field = sc->fields + i;
            if(field->type->type <= type_var && !field->isArray) {
                copy_object(o->node + i, create_object(1, field->type->type));
            }
        }
    }

    push_object(o);
    return o;
}

sharp_object* create_object(sharp_class* sc, Int size, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->type = type_class;
        o->arrayFlag = 1;
        SET_INFO(o->info, sc->address, gc_normal);

        o->node = calloc_mem<object>(size, sizeof(object), unsafe);
        push_object(o);
        return o;
    }

    return nullptr;
}

void realloc_object(sharp_object *o, Int size, bool unsafe) {
    if(o) {
        if(size <= 0) {
            throw vm_exception("call to realloc() with size <= 0");
        }

        if(o->type <= type_var) {
            o->HEAD = realloc_mem<long double>(o->HEAD, sizeof(long double) * size, o->size, unsafe);

            if(size > o->size) {
                for(Int i = o->size; i < size; i++) {
                    o->HEAD[i] = 0;
                }
            }

        } else {
            o->node = realloc_mem<object>(o->node, sizeof(object) * size, o->size, unsafe);

            if(size > o->size) {
                for(Int i = o->size; i < size; i++) {
                    o->node[i].o = NULL;
                }
            }
        }

        o->size = size;
    }
}

Int sizeof_object(sharp_object *o) {
    double size =0;
    if(o != nullptr) {

        if(o->type <= type_var) {
            size += sizeof(double) * o->size;
        } else {
            for(uInt i = 0; i < o->size; i++) {
                if(o->node[i].o != nullptr) {
                    size += sizeof_object(o->node[i].o);
                }
            }

            size += sizeof(object) * o->size;
        }

        size += sizeof(sharp_object);
    }

    return size;
}

sharp_object* create_object(Int size, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->type = type_object;
        o->arrayFlag = 1;
        SET_GENERATION(o->info, gc_normal);

        o->node = calloc_mem<object>(size, sizeof(object), unsafe);
        push_object(o);
        return o;
    }

    return nullptr;
}

sharp_object* create_object(Int size, data_type type, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->type = type;
        o->arrayFlag = 1;
        SET_GENERATION(o->info, gc_normal);
        o->HEAD = calloc_mem<long double>(size, sizeof(long double), unsafe);

        push_object(o);
        return o;
    }

    return nullptr;
}

void cast_object(object *from, Int toClass) {
    if(!(from->o->type == type_class && is_type(from, toClass))) {
        stringstream ss;
        if(from->o->type == type_class) {
            ss << "invalid cast from: "
                  + vm.classes[CLASS(from->o->info)].fullName + " to: " + vm.classes[toClass].fullName;
        } else {
            auto clazz = vm.classes[CLASS(from->o->info)].fullName;
            switch(from->o->type) {
                case type_int8:
                    ss << "casting from _int8" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_int16:
                    ss << "casting from _int16" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_int32:
                    ss << "casting from _int32" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_int64:
                    ss << "casting from _int64" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint8:
                    ss << "casting from _uint8" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint16:
                    ss << "casting from _uint16" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint32:
                    ss << "casting from _uint32" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint64:
                    ss << "casting from _uint64" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_function_ptr:
                    ss << "casting from func_ptr*" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_object:
                    ss << "casting from object" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                default:
                    ss << "casted object does not match expected type";
                    break;
            }
        }

        vm_exception err(vm.ill_state_except, ss.str());
        enable_exception_flag(thread_self, true);
        throw err;
    }
}

void cast_numeric_array(object *from, Int toType) {
    if(!(from->o->type <= type_var && is_type(from, toType))) {
        stringstream ss;
        switch(from->o->type) {
            case type_int8:
                ss << "illegal cast from _int8" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_int16:
                ss << "illegal cast from _int16" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_int32:
                ss << "illegal cast from _int32" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_int64:
                ss << "illegal cast from _int64" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint8:
                ss << "illegal cast from _uint8" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint16:
                ss << "illegal cast from _uint16" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint32:
                ss << "illegal cast from _uint32" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint64:
                ss << "illegal cast from _uint64" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_function_ptr:
                ss << "illegal cast from func_ptr*" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_object:
                ss << "illegal cast from object" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_class: {
                auto clazz = vm.classes[CLASS(from->o->info)].fullName;
                ss << "illegal cast from class " << clazz;
                break;
            }
            default:
                ss << "illegal cast object does not match expected type";
                break;
        }

        vm_exception err(vm.ill_state_except, ss.str());
        enable_exception_flag(thread_self, true);
        throw err;
    }
}

// ---------------------------------------------------------------------------
// lock(x) { ... }
//
//   LOCK   -> lock_object(x)       at the start of the block
//   ULOCK  -> unlock_object(x)     at its end (the compiler also emits it in
//                                  front of a return / break / continue that
//                                  leaves the block)
//
// Two things a lock block has to get right, and did not:
//
//   1. Nesting. lock(x) { lock(x) { } ...rest... } - the same fiber may enter
//      a lock it already holds. The lock counts how often (fib_mutex::count)
//      and is only given back when the outermost block ends. It used to be
//      released by the first ULOCK, so "rest" ran unprotected.
//
//   2. Exceptions. An exception that leaves the block skips its ULOCK. Every
//      block a fiber is inside of is therefore written down (fiber::held, with
//      the call depth and pc of its LOCK); when an exception is caught further
//      out, or unwinds a function, the blocks it left are released by
//      release_locks_left_by_exception(). Before this the fiber kept the lock
//      until it ended.
//
// C++ comparison: std::lock_guard<std::recursive_mutex> - released by leaving
// the scope in any way, and re-entrant for the owner.
// ---------------------------------------------------------------------------

static void note_lock_held(fiber *task, fib_mutex *mut) {
    if(task->heldCount == task->heldCapacity) {
        Int capacity = task->heldCapacity == 0 ? 4 : task->heldCapacity * 2;
        held_lock *grown = (held_lock*)realloc(task->held, sizeof(held_lock) * capacity);
        if(grown == nullptr) return;    // out of memory: this block is simply not tracked
        task->held = grown;
        task->heldCapacity = capacity;
    }

    held_lock &entry = task->held[task->heldCount++];
    entry.mut = mut;
    entry.depth = task->callFramePtr;
    entry.pc = (Int)(task->pc - task->rom);
}

/** Forget the innermost block of `task` that holds `mut` (its ULOCK ran). */
static void forget_lock_held(fiber *task, fib_mutex *mut) {
    for(Int i = task->heldCount - 1; i >= 0; i--) {
        if(task->held[i].mut == mut) {
            for(Int j = i; j < task->heldCount - 1; j++)
                task->held[j] = task->held[j + 1];
            task->heldCount--;
            return;
        }
    }
}

/** One block less; the lock is free again when it was the last one. */
static void leave_lock(fiber *task, fib_mutex *mut) {
    if(mut->id != task->id) return;

    if(--mut->count > 0) return;    // still inside an outer lock(x) block

    mut->count = 0;
    mut->id = -1;

    // fibers that failed to take this lock are waiting to be told it is free
    sched_lock_released(mut);
}

void unlock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;

    if(mut->id == task->id) {
        forget_lock_held(task, mut);
        leave_lock(task, mut);
    }
}

/**
 * An exception is leaving code of the current fiber. Give back the locks of
 * every lock block it leaves:
 *
 *   - blocks entered in functions ABOVE `depth` (those functions are gone), and
 *   - blocks entered in the function at `depth` at a pc inside
 *     [fromPc, toPc] - the try block whose handler is about to run. Blocks
 *     nest, so a lock taken inside that try block also ends inside it.
 *
 * When the function at `depth` itself is unwound (nothing in it catches the
 * exception) the range is the whole function: fromPc = 0, toPc = max.
 *
 * Called by catch_exception() (virtual_machine.cpp), which both the
 * interpreter and the JIT use.
 */
void release_locks_left_by_exception(fiber *task, Int depth, Int fromPc, Int toPc) {
    for(Int i = task->heldCount - 1; i >= 0; i--) {
        held_lock &entry = task->held[i];

        if(entry.depth > depth
           || (entry.depth == depth && entry.pc >= fromPc && entry.pc <= toPc)) {
            fib_mutex *mut = entry.mut;

            for(Int j = i; j < task->heldCount - 1; j++)
                task->held[j] = task->held[j + 1];
            task->heldCount--;

            leave_lock(task, mut);
        }
    }
}

bool lock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;
    auto thread = thread_self;

    if(mut->id == task->id) {
        // already ours: one more block inside the same lock
        mut->count++;
        note_lock_held(task, mut);
        return false;
    } else {
        task->f_lock = mut;
    }

    lock_mut.lock();
    if(mut->id == -1) {
        mut->id = task->id;
        mut->count = 1;
        task->f_lock = nullptr;
        lock_mut.unlock();
        note_lock_held(task, mut);
        TELEMETRY_COUNT(tm_lock_acquired)
    } else {
        lock_mut.unlock();
        TELEMETRY_COUNT(tm_lock_contended)
        enable_context_switch(thread, true); // todo: check if in c++ env if so throw an exception
        return true;
    }

    return false;
}

extern void populate_string(string &s, sharp_object *o) {
    for(Int i = 0; i < o->size; i++)
        s += (unsigned char)o->HEAD[i];
}
