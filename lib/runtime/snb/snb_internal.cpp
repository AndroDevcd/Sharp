//
// Created by bknun on 7/14/2023.
//

#include "snb_internal.h"
#include "../memory//sharp_object.h"
#include "snb_api.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../virtual_machine.h"
#include "../reflect/reflect_helpers.h"
#include "../error/vm_exception.h"
#include "../../core/thread_state.h"
#include "../../core/opcode/opcode_macros.h"
#include "../memory/garbage_collector.h"

void ref_increment(SharpObject obj) {
    if(obj) inc_ref(((object*)(obj))->o)
}

void ref_decrement(SharpObject obj) {
    if(obj) dec_ref(((object*)(obj))->o)
}

double* get_local_number_at(int32_t address) {
    return &(thread_self->task->fp+address)->var;
}

SharpObject get_local_object_at(int32_t address) {
    return &(thread_self->task->fp+address)->obj;
}

SharpObject get_field(SharpObject obj, const char* name) {
    object *lastField = NULL, *tmp;
    Int len = strlen(name);
    if(obj != NULL) {
        lastField = ((object*)obj);
        stringstream fieldName;
        for(long i = 0; i < len; i++) {
            if(name[i] != '.')
                fieldName << name[i];

            if(name[i] == '.' || (i + 1) >= len){
                string f = fieldName.str();
                tmp = resolve_field(fieldName.str(), lastField->o);
                if(tmp == NULL) {
                    return NULL;
                }

                lastField = tmp;
                fieldName.str("");
            }
        }
    }

    return lastField;
}

long double* get_raw_number_data(SharpObject *obj) {
    if(obj && ((object*)obj)->o && ((object*)obj)->o->type <= type_var) {
        return ((object*)obj)->o->HEAD;
    }

    return nullptr;
}

double pop_number() {
    try {
        auto num = (thread_self->task->sp)->var;

        thread_self->task->sp--;
        if ((thread_self->task->sp - thread_self->task->stack) < 0) {
            throw vm_exception(vm.thread_stack_except, "the thread stack cannot be negative");
        }

        return num;
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return INVALID_NUMBER;
    }
}

SharpObject pop_object() {
    try {
        auto obj = &(thread_self->task->sp)->obj;

        thread_self->task->sp--;
        if ((thread_self->task->sp - thread_self->task->stack) < 0) {
            throw vm_exception(vm.thread_stack_except, "the thread stack cannot be negative");
        }

        return obj;
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return nullptr;
    }
}

uint32_t get_size(SharpObject obj) {
    if(obj && ((object*)obj)->o) {
        return ((object*)obj)->o->size;
    }

    return INVALID_NUMBER;
}

void assign_object(SharpObject dest, SharpObject src) {
    copy_object((object*)dest, (object*)src);
}

SharpObject get_static_class(const char *name) {
    auto sc = locate_class(name);
    if(sc) return vm.staticHeap + sc->address;
    else return nullptr;
}

void new_array(int32_t size, data_type type) {
    try {
        auto task = thread_self->task;
        grow_stack
        stack_overflow_check
        copy_object(
                &push_stack_object,
                create_object(size, type)
        );
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

void new_class(const char *name) {
    try {
        auto sc = locate_class(name);
        auto task = thread_self->task;
        grow_stack
        stack_overflow_check
        copy_object(
                &push_stack_object,
                create_object(sc)
        );
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

void new_object_array(int32_t size) {
    try {
        auto task = thread_self->task;
        grow_stack
        stack_overflow_check
        copy_object(
                &push_stack_object,
                create_object(size)
        );
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

void new_class_array(const char *name, int32_t size) {
    try {
        auto sc = locate_class(name);
        auto task = thread_self->task;
        grow_stack
        stack_overflow_check
        copy_object(
                &push_stack_object,
                create_object(sc, (Int)size)
        );
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

SharpObject get_object_at(SharpObject obj, int32_t index) {
    try {
        object *data = (object*)obj;
        if(data == NULL || data->o == NULL) {
            throw vm_exception(vm.nullptr_except, "");
        }

        if(index < 0 || index >= data->o->size) {
            stringstream ss;
            ss << "index out of bounds accessing array at: " << index << " where size is: " << data->o->size;
            throw vm_exception(vm.bounds_except, ss.str());
        }

        return data->o->node+index;
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return nullptr;
    }
}

void push_number_to_stack(double value) {
    try {
        auto task = thread_self->task;
        grow_stack
        stack_overflow_check
        push_stack_number = value;
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

void push_object_to_stack(SharpObject value) {
    try {
        auto task = thread_self->task;
        grow_stack
        stack_overflow_check
        copy_object(&push_stack_object, (object*)value);
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

void call(int32_t address) {
    try {
        prepare_method(address);
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

// should only be used at beginning of native function!!
int32_t allocate_stack_space(int32_t size) {
    try {
        auto task = thread_self->task;
        grow_stack_for(size)
        stack_overflow_check_for(size)
        thread_self->task->sp += size;
        return thread_self->task->sp - thread_self->task->stack;
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return -1;
    }
}

int32_t allocate_local_variable() {
    try {
        auto task = thread_self->task;
        grow_stack_for(1)
        stack_overflow_check_for(1)
        thread_self->task->sp++;
        return (thread_self->task->sp - thread_self->task->stack) - (thread_self->task->fp - thread_self->task->stack);
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return INVALID_NUMBER;
    }
}

bool is_exception_thrown() {
    return hasSignal(thread_self->signal, tsig_except);
}

const char* get_class_name(SharpObject obj) {
    try {
        object *data = (object*)obj;
        if(data == NULL || data->o == NULL) {
            throw vm_exception(vm.nullptr_except, "");
        }

        if(data->o->type == type_class)
            return vm.classes[CLASS(data->o->info)].fullName.c_str();
        else return NULL;
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
        return nullptr;
    }
}

void prepare_exception(SharpObject exception) {
    copy_object(&thread_self->task->exceptionObject, (object*)exception);
    sendSignal(thread_self->signal, tsig_except, 1);
}

void clear_exception() {
    copy_object(&thread_self->task->exceptionObject, (sharp_object*)NULL);
    sendSignal(thread_self->signal, tsig_except, 0);
}

SharpObject get_exception_object() {
    if(thread_self->task->exceptionObject.o)
        return &thread_self->task->exceptionObject;
    else return nullptr;
}

void dup_stack() {
    try {
        auto task = thread_self->task;
        auto data = (Int)&task->sp->obj;
        copy_object(&push_stack_object, (object*)data);
    } catch(vm_exception &e) {
        sendSignal(thread_self->signal, tsig_except, 1);
    }
}

void return_object(SharpObject obj) {
    auto task = thread_self->task;
    copy_object(&task->fp->obj, (object*)obj);
}

void return_number(double num) {
    auto task = thread_self->task;
    task->fp->var = num;
}

void return_call() {
    return_method();
}

bool validate_app_id(int id) {
    if(vm.manifest.applicationId != id) {
        vm_exception e("error invalid app id: external library is out of sync with source code. Please update your c++ mapping files!");
        sendSignal(thread_self->signal, tsig_except, 1);
        return false;
    }

    return true;
}

void *lib_procs[LIB_PROCS_SIZE] =
        {
                (void*)ref_increment,
                (void*)ref_decrement,
                (void*)get_local_number_at,
                (void*)get_local_object_at,
                (void*)get_field,
                (void*)get_raw_number_data,
                (void*)pop_number,
                (void*)pop_object,
                (void*)get_size,
                (void*)assign_object,
                (void*)get_static_class,
                (void*)new_array,
                (void*)new_class,
                (void*)new_object_array,
                (void*)new_class_array,
                (void*)get_object_at,
                (void*)push_number_to_stack,
                (void*)push_object_to_stack,
                (void*)call,
                (void*)is_exception_thrown,
                (void*)get_class_name,
                (void*)prepare_exception,
                (void*)clear_exception,
                (void*)get_exception_object,
                (void*)allocate_stack_space,
                (void*)allocate_local_variable,
                (void*)dup_stack,
                (void*)return_object,
                (void*)return_number,
                (void*)return_call,
                (void*)validate_app_id,
        };

int initializeLibrary(lib_init linit) {
    return linit(
        lib_procs,
        LIB_PROCS_SIZE,
        SNB_API_VERS
    );
}