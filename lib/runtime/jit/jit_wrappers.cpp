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


#pragma GCC push_options
#pragma GCC optimize ("O0")
// Fiber/Task wrapper functions
void jit_growStack(fiber* task, Int requiredSize) {
    task->growStack(requiredSize);
}

// Exception wrapper functions
void jit_throwException(sharp_class* exceptionClass, const char* message) {
    auto error = vm_exception(exceptionClass, message);
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
#pragma GCC pop_options