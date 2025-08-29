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

// Debug wrapper functions
void jit_instructionStart(opcode_instr opcode, int pc) {
    if(pc == 3) {
        int i = 0;
        // Debug checkpoint for ISTORE instruction
    }
}
#pragma GCC pop_options