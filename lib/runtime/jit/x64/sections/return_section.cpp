//
// Return Section for x86-64 JIT Compiler
// Handles centralized function return and cleanup
//

#include "../x64_compiler.h"
#include "../../../multitasking/thread/sharp_thread.h"
#include "../../../multitasking/fiber/fiber.h"
#include "../../../multitasking/thread/thread_controller.h"
#include "../../../virtual_machine.h"
#include "../../../../core/opcode/opcode.h"
#include "../../jit_wrappers.h"
#include <cstddef>  // For offsetof

using namespace asmjit;

/**
 * Every exit from a compiled function comes through here with the
 * jit_return_code in eax.
 *
 *   returnFromFunction(code):
 *       if(code != JIT_CONTEXT_SWITCH && code != JIT_KILL) {
 *           // the function is finished (normally or by an uncaught exception):
 *           // pop its VM frame, exactly like the interpreter's RET does
 *           if(code == JIT_EXCEPTION)
 *               enable_exception_flag(thread, true);
 *
 *           return_method();
 *           if(code != JIT_EXCEPTION)
 *               task->pc++;      // continue after the CALL in the caller. With an
 *                                // exception pending task->pc stays ON the CALL,
 *                                // catch_exception() needs that pc to find the
 *                                // try block the call was made from.
 *       }
 *
 *       // context switch / kill: the VM frame stays, task->pc already says
 *       // where to resume
 *       return code;
 *
 * REGISTERS USED: everything a C call destroys, spill slot
 */
void X64Compiler::generateReturnSection() {
    assembler->bind(returnFromFunctionLabel);

    Label skipVmUpdates = assembler->newLabel();
    x86::Mem savedCode = x86::dword_ptr(x86::rsp, SPILL_SLOT);

    assembler->cmp(x86::eax, JIT_CONTEXT_SWITCH);
    assembler->je(skipVmUpdates);
    assembler->cmp(x86::eax, JIT_KILL);
    assembler->je(skipVmUpdates);

    // The calls below destroy eax, keep the return code in our frame
    assembler->mov(savedCode, x86::eax);

    // if(code == JIT_EXCEPTION) enable_exception_flag(thread, true);
    Label skipExceptionFlag = assembler->newLabel();
    assembler->cmp(x86::eax, JIT_EXCEPTION);
    assembler->jne(skipExceptionFlag);
    assembler->mov(arg0, threadPtr);
    assembler->mov(arg1.r32(), 1);
    callFunction(reinterpret_cast<void*>(jit_enableExceptionFlag));
    assembler->bind(skipExceptionFlag);

    // return_method();
    callFunction(reinterpret_cast<void*>(return_method));

    // if(code != JIT_EXCEPTION) task->pc++;   (pc is an opcode_instr*, so that is +4 bytes)
    Label keepPc = assembler->newLabel();
    assembler->mov(x86::eax, savedCode);
    assembler->cmp(x86::eax, JIT_EXCEPTION);
    assembler->je(keepPc);
    loadTask(tmp1);
    assembler->add(x86::qword_ptr(tmp1, offsetof(fiber, pc)), (int)sizeof(opcode_instr));
    assembler->bind(keepPc);

    assembler->bind(skipVmUpdates);

    // Function epilogue - mirror image of setupFunctionPrologue()
    assembler->add(x86::rsp, FRAME_LOCALS);
    assembler->pop(pcReg);           // r15
    assembler->pop(jumpTablePtr);    // r14
    assembler->pop(jitFunctionPtr);  // r13
    assembler->pop(registersPtr);    // r12
    assembler->pop(threadPtr);       // rbx
    assembler->pop(x86::rbp);
    assembler->ret();
}
