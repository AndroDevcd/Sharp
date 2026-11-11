//
// Throw Sections for x86-64 JIT Compiler
// Shared out-of-line code that raises a VM exception and enters the handler.
//
// In the interpreter these are either
//     vm_exception err(vm.xxx_except, ""); enable_exception_flag(thread, true); goto catch_exception;
// or a C++ `throw vm_exception(...)` that the try/catch around the main loop
// turns into exactly the same thing. Compiled code cannot throw C++ exceptions,
// so it always does the first form.
//
// All of them expect pcReg to hold the pc of the faulting instruction (it is set
// at the start of every instruction).
//

#include "../x64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../../virtual_machine.h"
#include "../../jit_wrappers.h"

using namespace asmjit;

/**
 * Reached when a jump lands on something that is not the start of an
 * instruction (second word of a two-word opcode, or outside the function).
 *
 *   vm_exception err(vm.invalid_operation_except, "Attempt to branch to invalid code section");
 *   enable_exception_flag(thread, true);
 *   goto catch_exception;
 */
void X64Compiler::generateIllegalBranchSection() {
    assembler->bind(illegalBranchLabel);

    assembler->mov(arg0, reinterpret_cast<uint64_t>(vm.invalid_operation_except));
    assembler->mov(arg1, reinterpret_cast<uint64_t>("Attempt to branch to invalid code section"));
    callFunction(reinterpret_cast<void*>(jit_throwException));

    assembler->mov(arg0, threadPtr);
    assembler->mov(arg1.r32(), 1);
    callFunction(reinterpret_cast<void*>(jit_enableExceptionFlag));

    assembler->jmp(catchExceptionLabel);
}

/**
 * Failure path of require_object / require_object_with_value /
 * require_numeric_object_with_value:
 *
 *   vm_exception err(vm.nullptr_except, "");
 *   enable_exception_flag(thread, true);
 *   goto catch_exception;
 */
void X64Compiler::generateNullptrExceptionSection() {
    assembler->bind(nullptrExceptionLabel);

    assembler->mov(arg0, reinterpret_cast<uint64_t>(vm.nullptr_except));
    assembler->mov(arg1, reinterpret_cast<uint64_t>(""));
    callFunction(reinterpret_cast<void*>(jit_throwException));

    assembler->mov(arg0, threadPtr);
    assembler->mov(arg1.r32(), 1);
    callFunction(reinterpret_cast<void*>(jit_enableExceptionFlag));

    assembler->jmp(catchExceptionLabel);
}

/**
 * Failure path of stack_overflow_check_for(n):
 *
 *   throw vm_exception(vm.stack_overflow_except, "");
 */
void X64Compiler::generateStackOverflowSection() {
    assembler->bind(stackOverflowLabel);

    assembler->mov(arg0, reinterpret_cast<uint64_t>(vm.stack_overflow_except));
    assembler->mov(arg1, reinterpret_cast<uint64_t>(""));
    callFunction(reinterpret_cast<void*>(jit_throwException));

    assembler->mov(arg0, threadPtr);
    assembler->mov(arg1.r32(), 1);
    callFunction(reinterpret_cast<void*>(jit_enableExceptionFlag));

    assembler->jmp(catchExceptionLabel);
}
