//
// The JIT compiler for x86-64 (System V calling convention: Linux, macOS on
// Intel). Turns the typed bytecode of one function into machine code.
//
// WHAT THE MACHINE CODE LOOKS LIKE
// --------------------------------
// Six CPU registers are reserved for the whole compiled function. They are all
// callee-saved, so calling into C++ does not disturb them:
//
//      r12   thread      sharp_thread*
//      r13   task        fiber*  (the running fiber: stack, pc, call frames)
//      r14   regs        vm_cell*  the fiber's ten VM registers (adx, cx ... egx)
//      r15   fp          stack_item*  first slot of this function's frame
//      rbx   sp          stack_item*  top of the stack
//      rbp   ptr         object*  the "object pointer" register of the VM
//
// fp, sp and ptr are COPIES of task->fp, task->sp and task->ptr. The interpreter
// reads and writes those three in memory on every instruction; compiled code
// keeps them in registers and
//
//      save_stack_registers()    writes sp and ptr back to the fiber    before any call into C++
//      load_stack_registers()    reads fp, sp and ptr from the fiber    after a call that may
//                                                                       have changed them
//
// Six more CPU registers hold the VM registers programs use most (their
// "homes"; the other four VM registers stay in the fiber's register array):
//
//      r8    egx         r9    ecx         r10   cx
//      r11   bmr         rsi   ebx         rdi   cmt
//
// These are NOT preserved by C++, so they are written to the register array
// before a call into C++ and read back after it - save_vm_registers() and
// load_vm_registers(); x64_generator.h explains the two states this gives
// ("register mode" and "memory mode").
//
// A bytecode instruction becomes a few machine instructions, with no dispatch
// and no decoding (the operands are constants in the machine code):
//
//      add ecx, ebx -> ecx          add r9, rsi
//
//      add ecx, ebx -> bmr          mov rax, r9
//                                   add rax, rsi
//                                   mov r11, rax
//
//      dadd ecx, ebx -> bmr         movq xmm0, r9
//                                   movq xmm1, rsi
//                                   addsd xmm0, xmm1
//                                   movq r11, xmm0
//
//      loadl ebx, fp+2              mov rsi, [r15 + 2*16]
//
//      getf ebx, slot 1             (null checks)  mov rax, [rbp]      ; the object
//                                   mov rcx, [rax]                     ; its slots
//                                   mov rsi, [rcx + 1*8]
//
// WHERE THE CODE IS
// -----------------
// This class is only the door the VM knocks on. The machine code is written by
// X64Generator - x64_generator.h has the map: sections/ for the parts every
// function has once (entry, dispatch, exception, exits, slow paths), opcode/
// for the instructions, one file per kind.
//
// WHAT STAYS EXACTLY AS IN THE INTERPRETER
// ----------------------------------------
//   - the fiber's stack and frames: a compiled function has the same VM frame,
//     so stack traces, exceptions and the garbage collector see no difference
//   - object references are only ever in stack slots, fields and task->ptr when
//     another thread can look (safe points, calls): the collector is unchanged
//   - every instruction boundary is a point the function can be entered at (the
//     jump table has an entry for each): continuing a fiber, entering a hot
//     loop from the interpreter and jumping to an exception handler all work by
//     setting task->pc and calling the function
//
// LEAVING COMPILED CODE (the return value, see jit_compiler.h)
//      JIT_OK               the function returned; its VM frame is popped
//      JIT_EXCEPTION        an exception left it; frame popped, flag set
//      JIT_CONTEXT_SWITCH   not finished: the fiber gives up the thread (or a
//                           callee did). Frame and task->pc stay; calling the
//                           function again continues at task->pc
//      JIT_KILL             the thread is stopping
//
// Slow things - allocation, calls, interrupts, casts, anything that can raise
// an exception - are calls to the functions in jit/jit_runtime.h.
//
// A function with an instruction this file does not handle is not compiled
// (compileFunction returns false) and keeps running in the interpreter.
// telemetry_jit_blocked() records which instruction it was.
//

#ifndef SHARP_X64_COMPILER_H
#define SHARP_X64_COMPILER_H

#include "../jit_compiler.h"
#include "../../../util/jit/asmjit/src/asmjit/x86.h"

class X64Compiler : public JitCompiler {
public:
    bool compileFunction(sharp_function* function, jit_compiled_function* output) override;
    void releaseCompiledFunction(jit_compiled_function* compiledFunc) override;

private:
    asmjit::JitRuntime runtime;     // owns the executable memory
};

#endif //SHARP_X64_COMPILER_H
