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

std::recursive_mutex copy_mut;
std::recursive_mutex lock_mut;

void init_struct(object *o) {
    o->o = nullptr;
}

void init_struct(sharp_object *o) {
    o->next = nullptr;
    o->type = 0;
    o->size = 0;
    o->info = 0;
    o->refCount = 0;
    o->HEAD = nullptr;
}

CXX11_INLINE void copy_object(object *to, object *from) {
    guard_mutex(copy_mut)
    if(to->o) dec_ref(to->o)
    to->o = from->o;
    if(to->o) inc_ref(to->o)
}

CXX11_INLINE void copy_object(object *to, sharp_object *from) {
    guard_mutex(copy_mut)
    if(to->o) dec_ref(to->o)
    to->o = from;
    if(to->o) inc_ref(to->o)
}

sharp_object* create_object(sharp_class* sc, bool unsafe) {
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

    init_struct(o);
    o->size = sc->instanceFields;
    o->refCount = invalid_references;
    o->type = type_class;
    SET_INFO(o->info, sc->address, gc_young);
    push_object(o);

    if(o->size > 0)
    {
        o->node = malloc_struct<object>(sizeof(object),  o->size, unsafe);
        for(Int i = 0; i < o->size; i++) {
            sharp_field *field = sc->fields + (sc->instanceFields + i);
            if(field->type->type <= type_var && !field->isArray) {
                copy_object(o->node + i, create_object(1, field->type->type));
            }
        }
    }

    return o;
}

sharp_object* create_object(Int size, bool unsafe) {
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->refCount = invalid_references;
        o->type = type_object;
        SET_GENERATION(o->info, gc_young);
        push_object(o);

        o->node = malloc_struct<object>(sizeof(object) * size, unsafe);
        return o;
    }

    return nullptr;
}

sharp_object* create_object(Int size, data_type type, bool unsafe) {
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->refCount = invalid_references;
        o->type = type;
        SET_GENERATION(o->info, gc_young);
        push_object(o);

        o->HEAD = malloc_mem<long double>(sizeof(long double) * size, unsafe);
        return o;
    }

    return nullptr;
}

void unlock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;

    if(mut->id == task->id) {
        mut->id = -1;
    }
}

void lock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;
    auto thread = thread_self;

    if(mut->id == task->id) {
        return;
    } else {
        task->f_lock = mut;
    }

    const int spinLimit = 100000000;
    int spins = 0;

    acquire_lock:
    while(mut->id != -1)
    {
        if(spins >= spinLimit)
            enable_context_switch(thread, true);

        if(thread->signal) {
            if(hasSignal(thread->signal, tsig_suspend))
                suspend_self();
            else {
                return;
            }
        }

        __usleep(0); // yield
        spins++;
    }

    lock_mut.lock();
    if(mut->id == -1) {
        mut->id = task->id;
        lock_mut.unlock();
    } else {
        lock_mut.unlock();
        goto acquire_lock;
    }
}
