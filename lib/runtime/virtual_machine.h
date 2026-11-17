//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_VIRTUAL_MACHINE_H
#define SHARP_VIRTUAL_MACHINE_H

#include "meta_data.h"
#include "exe/manifest.h"
#include "types/sharp_type.h"
#include "memory/sharp_object.h"
#include "error/vm_exception.h"

extern thread_local vm_cell *registers;

enum vm_state {
    VM_CREATED = 0,
    VM_RUNNING = 1,
    VM_SHUTTING_DOWN = 2,
    VM_TERMINATED = 3
};

struct virtual_machine {
    virtual_machine()
    :
            mf(),
            classes(nullptr),
            methods(nullptr),
            strings(nullptr),
            constants(nullptr),
            staticHeap(nullptr),
            exitVal(0),
            mdata(),
            state(VM_CREATED),
            nativeTypes(nullptr)
    {}

    manifest mf;
    int exitVal;
    sharp_class *classes;
    sharp_function *methods;
    string *strings;
    double *constants;
    object *staticHeap;
    object memoryExcept;
    meta_data mdata;
    sharp_type *nativeTypes;
    vm_state state;

    /**
     * Frequently used classes
     */
    sharp_class *throwable;
    sharp_class *stack_overflow_except;
    sharp_class *runtime_except;
    sharp_class *thread_stack_except;
    sharp_class *bounds_except;
    sharp_class *nullptr_except;
    sharp_class *class_cast_except;
    sharp_class *out_of_memory_except;
    sharp_class *invalid_operation_except;
    sharp_class *unsatisfied_link_except;
    sharp_class *incompatible_class_except;
    sharp_class *object_import_error;
    sharp_class *ill_state_except;
    sharp_class *string_class;
    sharp_class *int_class;
    sharp_class *char_class;
    sharp_class *bool_class;
    sharp_class *short_class;
    sharp_class *long_class;
    sharp_class *byte_class;
    sharp_class *ushort_class;
    sharp_class *double_class;
    sharp_class *uint_class;
    sharp_class *uchar_class;
    sharp_class *ulong_class;
    sharp_class *stack_sate;
    sharp_class *thread_class;
    sharp_class *exception_class;
    sharp_class *error_class;
    sharp_class *char_array_class;
    sharp_class *fiber_class;
    sharp_class *reflect_class;
    sharp_class *reflect_field;
    sharp_class *reflect_function;
    sharp_class *reflect_data_type;
    sharp_class *reflect_func_params;
    sharp_class *reflect_description;
};

extern virtual_machine vm;

void main_vm_loop(sharp_function *frame);
void exec_interrupt(Int interrupt);         // interrupts/interrupt_table.cpp: dispatch an interrupt to its handler
void prepare_method(Int address);
void invoke_delegate(Int address, Int argSize, bool staticCall);
bool catch_exception();
bool return_method();
void invoke_next_frame(sharp_function *frame, bool isInterpreter);
sharp_function* get_next_frame(sharp_function *current);
sharp_function* next_rebuild_frame();

/**
 * a % b as the VM defines it (both sides cut down to whole numbers first).
 *
 * In C++ a remainder by zero is not an error value but a hardware trap: the
 * whole VM died with SIGFPE and no Sharp exception could be caught. It now
 * raises invalid_operation_exception like any other run time error.
 * (INT64_MIN % -1 traps the same way on x86; its mathematical result is 0.)
 *
 * Used by MOD / IMOD / MODL / IMODL in the interpreter.
 */
inline Int vm_remainder(Int left, Int right) {
    if(right == 0)
        throw vm_exception(vm.invalid_operation_except, "remainder of a division by zero");
    if(right == -1)
        return 0;
    return left % right;
}

#endif //SHARP_VIRTUAL_MACHINE_H
