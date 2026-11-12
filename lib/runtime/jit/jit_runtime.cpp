//
// What compiled code calls back into the runtime for. See jit_runtime.h for
// the three rules; every function here is the body of the interpreter's
// handler of the same instruction, with "throw" turned into "set the exception
// flag".
//

#include "jit_runtime.h"
#include "jit_compiler.h"
#include "../virtual_machine.h"
#include "../error/vm_exception.h"
#include "../memory/garbage_collector.h"
#include "../memory/tracing_collector.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/thread/thread_controller.h"
#include "../multitasking/fiber/fiber.h"
#include "../reflect/reflect_helpers.h"
#include "../termios.h"
#include "../../core/opcode/opcode_macros.h"
#include "../../core/thread_state.h"
#include "../telemetry/telemetry.h"
#include <cmath>

/** run `code`; a vm_exception becomes the exception flag of the running thread */
#define GUARDED(code) \
    try { code } catch(vm_exception &e) { enable_exception_flag(thread_self, true); }

// ---------------------------------------------------------------------------------
// Calls and returns
// ---------------------------------------------------------------------------------

void jit_rt_call(Int address) {
    // prepare_method() builds the callee's frame and, because the caller is
    // compiled code, runs the callee to its end before it comes back.
    GUARDED( prepare_method(address); )
}

void jit_rt_call_dynamic(Int address) {
    GUARDED(
        if(address <= 0 || address >= vm.mf.methods) {
            stringstream ss;
            ss << "invalid call to method (@" << address << ") out of range!";
            throw vm_exception(ss.str());
        }

        prepare_method(address);
    )
}

void jit_rt_invoke_delegate(Int address, Int argSize, Int staticCall) {
    GUARDED( invoke_delegate(address, argSize, staticCall != 0); )
}

Int jit_rt_leave(sharp_thread *thread, Int code) {
    if(code == JIT_EXCEPTION)
        enable_exception_flag(thread, true);

    return_method();

    // Continue after the CALL in the caller. With an exception pending task->pc
    // stays ON the call: catch_exception() needs that pc to find the try block
    // the call was made from.
    if(code != JIT_EXCEPTION)
        thread->task->pc++;

    return code;
}

void jit_rt_rebuild_above(jit_compiled_function *fun) {
    sharp_function *frame = get_next_frame(fun->originalFunction);
    invoke_next_frame(frame, false);
}

// ---------------------------------------------------------------------------------
// Signals and exceptions
// ---------------------------------------------------------------------------------

Int jit_rt_safepoint(sharp_thread *thread) {
    // the same tests in the same order as `state_check:` in main_vm_loop
    if(hasSignal(thread->signal, tsig_gc))
        tgc_safepoint();
    if(hasSignal(thread->signal, tsig_suspend))
        suspend_self();
    if(hasSignal(thread->signal, tsig_except))
        return jit_exception;
    if(hasSignal(thread->signal, tsig_context_switch) && thread->nativeCalls == 0)
        return jit_switch;
    if(hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED)
        return jit_kill;

    return jit_continue;
}

Int jit_rt_catch(sharp_thread *thread) {
    if(thread->state == THREAD_KILLED) {
        enable_exception_flag(thread, true);
        return 2;
    }

    try {
        return catch_exception() ? 1 : 0;
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
        return 0;
    }
}

void jit_rt_null_pointer(sharp_thread *thread) {
    vm_exception err(vm.nullptr_except, "");
    enable_exception_flag(thread, true);
}

void jit_rt_out_of_bounds(sharp_thread *thread, Int index) {
    fiber *task = thread->task;
    stringstream ss;
    ss << "Access to Object at: " << index << " size is "
       << (task->ptr != NULL && task->ptr->o != NULL ? task->ptr->o->size : 0);
    vm_exception err(vm.bounds_except, ss.str());
    enable_exception_flag(thread, true);
}

void jit_rt_throw(sharp_thread *thread) {
    fiber *task = thread->task;
    copy_object(&task->exceptionObject, pop_stack_object.o);
    enable_exception_flag(thread, true);
}

void jit_rt_grow_stack(sharp_thread *thread) {
    fiber *task = thread->task;
    GUARDED(
        grow_stack
        stack_overflow_check
    )
}

// ---------------------------------------------------------------------------------
// Objects
// ---------------------------------------------------------------------------------

sharp_object* jit_rt_new_class(Int classAddress) {
    GUARDED( return create_object(&vm.classes[classAddress]); )
    return nullptr;
}

sharp_object* jit_rt_new_number_array(Int size, Int type) {
    GUARDED( return create_object(size, (data_type)type); )
    return nullptr;
}

sharp_object* jit_rt_new_object_array(Int size) {
    GUARDED( return create_object(size); )
    return nullptr;
}

sharp_object* jit_rt_new_class_array(Int classAddress, Int size) {
    GUARDED( return create_object(vm.classes + classAddress, size); )
    return nullptr;
}

sharp_object* jit_rt_new_string(Int stringIndex) {
    GUARDED(
        string &text = vm.strings[stringIndex];
        sharp_object *o = create_object(text.size(), type_int8);
        if(text.size() > 0)
            assign_string_field(o, text);
        return o;
    )
    return nullptr;
}

/** the object pointer must point at an object: raises nullptr_exception and gives false otherwise */
static bool object_present(sharp_thread *thread) {
    fiber *task = thread->task;
    if(task->ptr == NULL || task->ptr->o == NULL) {
        jit_rt_null_pointer(thread);
        return false;
    }
    return true;
}

void jit_rt_cast(Int classAddress) {
    if(!object_present(thread_self)) return;
    GUARDED( cast_object(thread_self->task->ptr, classAddress); )
}

void jit_rt_cast_number_array(Int type) {
    if(!object_present(thread_self)) return;
    GUARDED( cast_numeric_array(thread_self->task->ptr, type); )
}

Int jit_rt_is(Int type) {
    GUARDED( return is_type(thread_self->task->ptr, type); )
    return 0;
}

Int jit_rt_lock(sharp_thread *thread) {
    if(!object_present(thread)) return 0;
    GUARDED( return lock_object(thread->task->ptr->o) ? 1 : 0; )
    return 0;
}

void jit_rt_unlock(sharp_thread *thread) {
    if(!object_present(thread)) return;
    GUARDED( unlock_object(thread->task->ptr->o); )
}

void jit_rt_swap(sharp_thread *thread) {
    fiber *task = thread->task;

    if((task->sp - task->stack) >= 2) {
        auto swappedObj = task->sp->obj.o;
        (task->sp)->obj.o = (task->sp-1)->obj.o;
        (task->sp-1)->obj.o = swappedObj;
        vm_cell swappedVar = task->sp->var;
        (task->sp)->var = (task->sp-1)->var;
        (task->sp-1)->var = swappedVar;
    } else {
        vm_exception err(vm.thread_stack_except, "Illegal stack swap, not enough data");
        enable_exception_flag(thread, true);
    }
}

// ---------------------------------------------------------------------------------
// Numbers
// ---------------------------------------------------------------------------------

Int jit_rt_remainder(Int left, Int right) {
    GUARDED( return vm_remainder(left, right); )
    return 0;
}

Int jit_rt_decimal_to_integer(double value) {
    return decimal_to_integer(value);
}

double jit_rt_power(double base, double exponent) {
    return pow(base, exponent);
}

// ---------------------------------------------------------------------------------
// Everything else
// ---------------------------------------------------------------------------------

Int jit_rt_interrupt(Int flag) {
    TELEMETRY_SCOPE(tm_mode_interrupt)
    TELEMETRY_COUNT(tm_interrupts)
    GUARDED( exec_interrupt(flag); )

    if(vm.state == VM_TERMINATED) {
        send_interrupt_signal(thread_self);
        return 1;
    }
    return 0;
}

void jit_rt_put_integer(Int value) {
    cout << value;
}

void jit_rt_put_decimal(double value) {
    cout << value;
}

void jit_rt_put_character(Int value) {
    printf("%c", (char)value);
}

Int jit_rt_get_character(Int echo) {
    gc_safe_region waitingForKey;
    return echo != 0 ? getche() : getch();
}

void jit_rt_sleep(Int time) {
    __os_sleep(time);
}
