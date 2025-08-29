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
    o->arrayFlag = 0;
    o->refCount = 0;
    o->HEAD = nullptr;
}

void copy_object(object *to, object *from) {
    guard_mutex(copy_mut)
    if(to->o) dec_ref(to->o)
    to->o = from ? from->o : nullptr;
    if(to->o) inc_ref(to->o)
}

void copy_object(object *to, sharp_object *from) {
    guard_mutex(copy_mut)
    if(to->o) dec_ref(to->o)
    to->o = from;
    if(to->o) inc_ref(to->o)
}

sharp_object* create_static_object(sharp_class* sc, bool unsafe) {
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

    init_struct(o);
    o->size = sc->staticFields;
    o->type = type_class;
    SET_INFO(o->info, sc->address, gc_young);

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
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

    init_struct(o);
    o->size = sc->instanceFields;
    o->type = type_class;
    SET_INFO(o->info, sc->address, gc_young);

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
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->type = type_class;
        o->arrayFlag = 1;
        SET_INFO(o->info, sc->address, gc_young);

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

        guard_mutex(copy_mut)
        if(o->type <= type_var) {
            o->HEAD = realloc_mem<long double>(o->HEAD, sizeof(long double) * size, o->size, unsafe);

            if(size > o->size) {
                for(Int i = o->size; i < size; i++) {
                    o->HEAD[i] = 0;
                }
            }

        } else {
            if(size < o->size) {
                for(size_t i = size; i < o->size; i++) {
                    if(o->node[i].o != nullptr) {
                        dec_ref(o->node[i].o)
                    }
                }
            }

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
    if(size > 0)
    {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

        init_struct(o);
        o->size = size;
        o->type = type_object;
        o->arrayFlag = 1;
        SET_GENERATION(o->info, gc_young);

        o->node = calloc_mem<object>(size, sizeof(object), unsafe);
        push_object(o);
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
        o->type = type;
        o->arrayFlag = 1;
        SET_GENERATION(o->info, gc_young);
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

void unlock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;

    if(mut->id == task->id) {
        mut->id = -1;
    }
}

bool lock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;
    auto thread = thread_self;

    if(mut->id == task->id) {
        return false;
    } else {
        task->f_lock = mut;
    }

    lock_mut.lock();
    if(mut->id == -1) {
        mut->id = task->id;
        task->f_lock = nullptr;
        lock_mut.unlock();
    } else {
        lock_mut.unlock();
        enable_context_switch(thread, true); // todo: check if in c++ env if so throw an exception
        return true;
    }

    return false;
}

extern void populate_string(string &s, sharp_object *o) {
    for(Int i = 0; i < o->size; i++)
        s += (unsigned char)o->HEAD[i];
}
