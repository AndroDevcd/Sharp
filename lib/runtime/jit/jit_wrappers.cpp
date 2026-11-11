//
// JIT Wrapper Functions Implementation
// Static C-style wrapper functions for calling C++ instance methods from JIT code
//

#include "jit_wrappers.h"
#include "../multitasking/fiber/fiber.h"
#include "../error/vm_exception.h"
#include "../virtual_machine.h"
#include "../multitasking/thread/thread_controller.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../memory/sharp_object.h"
#include "../../core/opcode/opcode_macros.h"
#include "../telemetry/telemetry.h"
#include "../reflect/reflect_helpers.h"
#include "../types/sharp_type.h"
#include "../termios.h"
#include <cmath>


#pragma GCC push_options
#pragma GCC optimize ("O0")
// Fiber/Task wrapper functions
void jit_growStack(fiber* task, Int requiredSize) {
    try {
        task->growStack(requiredSize);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

// Exception wrapper functions
void jit_throwException(sharp_class* exceptionClass, const char* message) {
    try {
        // constructing the exception pushes it onto the current task
        auto error = vm_exception(exceptionClass, message);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

bool jit_catchException() {
    try {
        return catch_exception();
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
        return false;
    }
}

void jit_enableExceptionFlag(sharp_thread* thread, bool enable) {
    try {
        enable_exception_flag(thread, enable);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

// Thread control wrapper functions
void jit_suspendSelf() {
    try {
        suspend_self();
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

// Object creation and manipulation wrapper functions
void* jit_createObject(sharp_class* sc) {
    try {
        return create_object(sc);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
        return nullptr;
    }
}

void jit_copyObject1(object* dest, sharp_object* src) {
    try {
        copy_object(dest, src);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

void jit_copyObject2(object* dest, object* src) {
    try {
        copy_object(dest, src);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

int jit_getObjectType(sharp_object* obj) {
    if (obj == nullptr) {
        return -1;  // Invalid type for null objects
    }
    return (int)obj->type;  // Access bit-field through C++ member access
}

// Function call wrapper functions
void jit_prepareMethod(int address) {
    try {
        prepare_method(address);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

/*
 * CALLD:
 *     if((data = (Int)regs[single_arg]) <= 0 || data >= vm.mf.methods) {
 *         stringstream ss;
 *         ss << "invalid call to method (@" << data << ") out of range!";
 *         throw vm_exception(ss.str());
 *     }
 *     prepare_method(data);
 */
void jit_prepareMethodDynamic(int inRegister) {
    try {
        Int address = (Int)registers[inRegister];
        if(address <= 0 || address >= vm.mf.methods) {
            stringstream ss;
            ss << "invalid call to method (@" << address << ") out of range!";
            throw vm_exception(ss.str());
        }

        prepare_method(address);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

/*
 * INVOKE_DELEGATE:
 *     invoke_delegate(single_arg, dual_raw_arg2, dual_raw_arg1);
 */
void jit_invokeDelegate(int address, int argSize, bool staticCall) {
    try {
        invoke_delegate(address, argSize, staticCall);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

/*
 * _INT:
 *     exec_interrupt(single_arg);
 *     if(vm.state == VM_TERMINATED) {
 *         send_interrupt_signal(thread);
 *         return;
 *     }
 *
 * Returns true when the vm terminated and the compiled function has to leave.
 */
bool jit_execInterrupt(int interrupt) {
    TELEMETRY_SCOPE(tm_mode_interrupt)
    TELEMETRY_COUNT(tm_interrupts)
    try {
        exec_interrupt(interrupt);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }

    if(vm.state == VM_TERMINATED) {
        send_interrupt_signal(thread_self);
        return true;
    }
    return false;
}

/*
 * CHECKLEN (only called when the fast inline check failed):
 *     if(regs[single_arg] < task->ptr->o->size && regs[single_arg] >= 0) { ok }
 *     else {
 *         stringstream ss;
 *         ss << "Access to Object at: " << regs[single_arg] << " size is " << task->ptr->o->size;
 *         throw vm_exception(vm.bounds_except, ss.str());
 *     }
 */
void jit_checkLen(int inRegister) {
    try {
        auto task = thread_self->task;
        if(!(registers[inRegister] < task->ptr->o->size && registers[inRegister] >= 0)) {
            stringstream ss;
            ss << "Access to Object at: " << registers[inRegister] << " size is " << task->ptr->o->size;
            throw vm_exception(vm.bounds_except, ss.str());
        }
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

/*
 * LOCK:
 *     if(lock_object(task->ptr->o)) return;
 */
bool jit_lockObject(sharp_object* o) {
    try {
        return lock_object(o);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
        return false;
    }
}

/*
 * ULOCK:
 *     unlock_object(task->ptr->o);
 */
void jit_unlockObject(sharp_object* o) {
    try {
        unlock_object(o);
    } catch(vm_exception &e) {
        enable_exception_flag(thread_self, true);
    }
}

// ---------------------------------------------------------------------------------
// Opcode wrappers. The comment above each one is the interpreter code it mirrors.
// JIT_GUARD turns a vm_exception into the thread's exception flag, compiled code
// checks that flag right after the call.
// ---------------------------------------------------------------------------------
#define JIT_GUARD(code) \
    try { code } catch(vm_exception &e) { enable_exception_flag(thread_self, true); }

// NEWARRAY: copy_object(&push_stack_object, create_object(regs[dual_arg1], (data_type)dual_arg2));
void jit_newArray(int inRegister, int ntype) {
    JIT_GUARD(
        auto task = thread_self->task;
        auto created = create_object(registers[inRegister], (data_type)ntype);
        copy_object(&push_stack_object, created);
    )
}

// NEWOBJARRAY: copy_object(&push_stack_object, create_object(regs[single_arg]));
void jit_newObjArray(int inRegister) {
    JIT_GUARD(
        auto task = thread_self->task;
        auto created = create_object(registers[inRegister]);
        copy_object(&push_stack_object, created);
    )
}

// NEWCLASSARRAY: copy_object(&push_stack_object, create_object(vm.classes + raw_arg2, (Int)regs[single_arg]));
void jit_newClassArray(int inRegister, int classAddress) {
    JIT_GUARD(
        auto task = thread_self->task;
        auto created = create_object(vm.classes + classAddress, (Int)registers[inRegister]);
        copy_object(&push_stack_object, created);
    )
}

// NEWSTRING:
//     copy_object(&push_stack_object, create_object(vm.strings[data].size(), type_int8));
//     if(vm.strings[data].size() > 0) assign_string_field(task->sp->obj.o, vm.strings[data]);
void jit_newString(int address) {
    JIT_GUARD(
        auto task = thread_self->task;
        auto created = create_object(vm.strings[address].size(), type_int8);
        copy_object(&push_stack_object, created);

        if(vm.strings[address].size() > 0)
            assign_string_field(task->sp->obj.o, vm.strings[address]);
    )
}

// CAST: cast_object(task->ptr, single_arg);
void jit_cast(int classAddress) {
    JIT_GUARD( cast_object(thread_self->task->ptr, classAddress); )
}

// VARCAST: cast_numeric_array(task->ptr, dual_arg1);
void jit_varCast(int varType) {
    JIT_GUARD( cast_numeric_array(thread_self->task->ptr, varType); )
}

// IS: regs[single_arg] = is_type(task->ptr, raw_arg2);
void jit_is(int outRegister, int type) {
    JIT_GUARD( registers[outRegister] = is_type(thread_self->task->ptr, type); )
}

// ITEST:
//     data = (Int)pop_stack_object.o;
//     regs[single_arg] = (sharp_object*)data == pop_stack_object.o;
void jit_itest(int outRegister) {
    auto task = thread_self->task;
    sharp_object *first = pop_stack_object.o;
    registers[outRegister] = first == pop_stack_object.o;
}

// SWAP: swaps the two items on top of the stack, throws if there are fewer than two
void jit_swap() {
    JIT_GUARD(
        auto task = thread_self->task;
        if((task->sp-task->stack) >= 2) {
            auto swappedObj = task->sp->obj.o;
            (task->sp)->obj.o = (task->sp-1)->obj.o;
            (task->sp-1)->obj.o = swappedObj;
            double swappedVar = task->sp->var;
            (task->sp)->var = (task->sp-1)->var;
            (task->sp-1)->var = swappedVar;
        } else {
            stringstream ss;
            ss << "Illegal stack swap, not enough data";
            throw vm_exception(vm.thread_stack_except, ss.str());
        }
    )
}

// PUT: cout << regs[single_arg];
void jit_put(int inRegister) {
    cout << registers[inRegister];
}

// PUTC: printf("%c", (char)regs[single_arg]);
void jit_putc(int inRegister) {
    printf("%c", (char)registers[inRegister]);
}

// GET: regs[single_arg] = (regs[CMT] != 0) ? getche() : getch();
void jit_get(int outRegister) {
    registers[outRegister] = (registers[CMT] != 0) ? getche() : getch();
}

// SLEEP: __os_sleep(regs[single_arg]);
void jit_sleep(int inRegister) {
    TELEMETRY_SCOPE(tm_mode_sleep)
    __os_sleep(registers[inRegister]);
}

// EXP: regs[triple_arg3]=pow(regs[triple_arg1], regs[triple_arg2]);
void jit_exp(int outRegister, int leftRegister, int rightRegister) {
    registers[outRegister] = pow(registers[leftRegister], registers[rightRegister]);
}

// MOVU64: regs[dual_arg1] = (uint64_t)regs[dual_arg2];
void jit_movu64(int outRegister, int registerToCast) {
    registers[outRegister] = (uint64_t)registers[registerToCast];
}

// AND: regs[BMR] = regs[dual_arg1] && regs[dual_arg2];
void jit_and(int leftRegister, int rightRegister) {
    registers[BMR] = registers[leftRegister] && registers[rightRegister];
}

// MOD: regs[triple_arg3] = (Int)regs[triple_arg1] % (Int)regs[triple_arg2];
void jit_mod(int outRegister, int leftRegister, int rightRegister) {
    registers[outRegister] = (Int)registers[leftRegister] % (Int)registers[rightRegister];
}

// IMOD: regs[single_arg] = (Int)regs[single_arg] % raw_arg2;
void jit_imod(int outRegister, int value) {
    registers[outRegister] = (Int)registers[outRegister] % value;
}

// MODL: (task->fp + dual_arg2)->var = ((Int)(task->fp + dual_arg2)->var) % (Int)regs[dual_arg1];
void jit_modl(int inRegister, int relFrameAddress) {
    auto task = thread_self->task;
    (task->fp + relFrameAddress)->var = ((Int)(task->fp + relFrameAddress)->var) % (Int)registers[inRegister];
}

// IMODL: (task->fp + single_arg)->var = ((Int)(task->fp + single_arg)->var) % raw_arg2;
void jit_imodl(int value, int relFrameAddress) {
    auto task = thread_self->task;
    (task->fp + relFrameAddress)->var = ((Int)(task->fp + relFrameAddress)->var) % value;
}

// Debug wrapper functions
void jit_instructionStart(opcode_instr opcode, int pc) {
    if(pc == 3) {
        int i = 0;
        // Debug checkpoint for ISTORE instruction
    }
}
#pragma GCC pop_options