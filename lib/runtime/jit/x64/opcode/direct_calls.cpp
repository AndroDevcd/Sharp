//
// Direct calls: compiled code calling compiled code without going through C++.
//
// THE SLOW WAY (opcode/calls.cpp, still used when the callee is not compiled yet)
//      machine code -> jit_rt_call() -> prepare_method() -> the callee's machine code
//      ... RET -> jit_rt_leave() -> return_method() -> back out through all of them
// Two C++ functions on the way in, two on the way out, each looking up the
// running thread again. That was 16-21 ns per call when everything else in a
// loop cost about 3.
//
// THE DIRECT WAY (this file)
// The caller's machine code does what prepare_method() does - record how to get
// back (a `frame` in task->frames), point fp / sp / pc at the callee - and then
// calls the callee's machine code itself. RET does what return_method() does.
//
// The VM's own call stack (task->frames, task->current, the stack slots) ends
// up EXACTLY as the slow way leaves it. Stack traces, exceptions, the garbage
// collector and continuing a fiber therefore need no change, and a direct call
// can be mixed freely with the slow way and the interpreter.
//
// C++ comparison: prepare_method() + return_method() inlined at the call site,
// the way a C++ compiler inlines a small function - minus everything that is
// known when the caller is compiled (the callee, its frame size, both pcs).
//
// Kept on the slow way: CALLD and INVOKE_DELEGATE (the callee is not known),
// RET with an exception, and builds with SAFE_EXECUTION or SHARP_TELEMETRY
// (prepare_method() checks and counts things there).
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

static_assert(sizeof(::frame) == 40, "x64 jit: direct calls index task->frames by 40 bytes");
static_assert(sizeof(opcode_instr) == 4, "x64 jit: an instruction word is 4 bytes");

/** rcx = task->frames + rcx   (rcx is an index) */
static void frame_address(x86::Assembler &a) {
    a.lea(rcx, x86::qword_ptr(rcx, rcx, 2));            // * 5
    a.shl(rcx, 3);                                      // * 8  -> * sizeof(frame)
    a.add(rcx, x86::qword_ptr(TASK, offsetof(fiber, frames)));
}

/**
 * CALL address        (see prepare_method() in virtual_machine.cpp)
 *
 *      if(!callee->isHighFrequency || no room on the stack || no room in frames)
 *          -> the slow way                                  (slow_call)
 *      task->calls++;
 *      frame *f = task->frames + ++task->callFramePtr;
 *      f->returnAddress = this function;  f->pc = this CALL;
 *      f->sp = (sp - callee->spOffset) - stack;  f->fp = fp - stack;  f->native = true;
 *      task->current = callee;
 *      fp = sp - callee->fpOffset;   sp += callee->frameStackOffset;
 *      task->rom = task->pc = callee->bytecode;
 *      call the callee's direct door                        (sections/entry.cpp)
 *
 * Nothing is passed and nothing is saved: thread, task, sp, fp, ptr and the VM
 * registers are in the CPU registers both functions agree on. Only rax, rcx
 * and rdx are used as scratch here - rsi and rdi are homes of VM registers.
 */
void X64Generator::direct_call(Int address) {
    sharp_function *callee = vm.methods + address;
    Label afterCall = a.newLabel();
    Label slowCall = request_slow_path(slow_call, pc, afterCall);

    // compiled yet? (it gets compiled by prepare_method on the slow way)
    a.mov(rax, (uint64_t)callee);
    a.cmp(x86::byte_ptr(rax, offsetof(sharp_function, isHighFrequency)), 0);
    a.je(slowCall);

    // grow_stack_for(callee->stackSize): sp + stackSize must stay below the end
    a.lea(rcx, x86::qword_ptr(SP, (int32_t)((callee->stackSize - 1) * 16)));
    a.cmp(rcx, fiber_field(offsetof(fiber, growAt)));
    a.jae(slowCall);

    // if((task->callFramePtr + 1) >= task->frameSize) task->growFrame();
    a.mov(rcx, fiber_field(offsetof(fiber, callFramePtr)));
    a.inc(rcx);
    a.cmp(rcx, fiber_field(offsetof(fiber, frameSize)));
    a.jae(slowCall);

    // ---- record how to get back
    a.inc(fiber_field(offsetof(fiber, calls)));
    a.mov(fiber_field(offsetof(fiber, callFramePtr)), rcx);
    frame_address(a);
    a.mov(x86::dword_ptr(rcx, offsetof(::frame, returnAddress)), (int32_t)function->address);
    a.mov(rdx, (uint64_t)(code + pc));
    a.mov(x86::qword_ptr(rcx, offsetof(::frame, pc)), rdx);
    a.mov(rdx, fiber_field(offsetof(fiber, stack)));
    a.lea(rax, x86::qword_ptr(SP, (int32_t)(-callee->spOffset * 16)));
    a.sub(rax, rdx);
    a.sar(rax, 4);                                      // bytes -> slots
    a.mov(x86::qword_ptr(rcx, offsetof(::frame, sp)), rax);
    a.mov(rax, FP);
    a.sub(rax, rdx);
    a.sar(rax, 4);
    a.mov(x86::qword_ptr(rcx, offsetof(::frame, fp)), rax);
    a.mov(x86::byte_ptr(rcx, offsetof(::frame, native)), 1);   // the caller is machine code

    // ---- the callee's frame
    a.mov(rax, (uint64_t)callee);
    a.mov(fiber_field(offsetof(fiber, current)), rax);
    a.lea(FP, x86::qword_ptr(SP, (int32_t)(-callee->fpOffset * 16)));
    a.mov(fiber_field(offsetof(fiber, fp)), FP);
    if(callee->frameStackOffset != 0)
        a.add(SP, (int32_t)(callee->frameStackOffset * 16));
    a.mov(rdx, (uint64_t)callee->bytecode);
    a.mov(fiber_field(offsetof(fiber, rom)), rdx);
    a.mov(fiber_field(offsetof(fiber, pc)), rdx);

    // ---- run it
    a.mov(rax, x86::qword_ptr(rax, offsetof(sharp_function, jfunc)));
    a.call(x86::qword_ptr(rax, offsetof(jit_compiled_function, directEntry)));

    // JIT_OK: the callee's RET (direct_return) has put OUR fp and sp back in
    // their registers. Anything else: sections/dispatch.cpp sorts it out.
    a.test(x86::eax, x86::eax);
    a.jnz(callNotFinished);
    a.cmp(thread_signal(), 0);                                 // check_state after a call
    a.jne(resumeFromRegisters);

    a.bind(afterCall);                                  // the slow way joins here
}

/**
 * RET (without an exception)      (see return_method() and jit_rt_leave())
 *
 *      if(task->calls == 1) -> the slow way: the fiber's first function is over
 *      task->calls--;
 *      frame *f = task->frames + task->callFramePtr--;
 *      task->current = vm.methods + f->returnAddress;   task->rom = its bytecode;
 *      task->pc = f->pc + 1;                            // after the CALL
 *      sp = stack + f->sp;   fp = stack + f->fp;        // the caller's: in the registers AND the fiber
 *      return JIT_OK;
 *
 * The VM registers stay in their homes: a result in ebx is simply still in rsi
 * when the caller continues.
 */
void X64Generator::direct_return() {
    Label lastFrame = a.newLabel();

    a.cmp(fiber_field(offsetof(fiber, calls)), 1);
    a.je(lastFrame);
    a.dec(fiber_field(offsetof(fiber, calls)));

    a.mov(rcx, fiber_field(offsetof(fiber, callFramePtr)));
    a.lea(rdx, x86::qword_ptr(rcx, -1));
    a.mov(fiber_field(offsetof(fiber, callFramePtr)), rdx);
    frame_address(a);

    a.movsxd(rax, x86::dword_ptr(rcx, offsetof(::frame, returnAddress)));
    a.imul(rax, rax, (int32_t)sizeof(sharp_function));
    a.mov(rdx, (uint64_t)vm.methods);
    a.add(rax, rdx);
    a.mov(fiber_field(offsetof(fiber, current)), rax);
    a.mov(rdx, x86::qword_ptr(rax, offsetof(sharp_function, bytecode)));
    a.mov(fiber_field(offsetof(fiber, rom)), rdx);

    a.mov(rdx, x86::qword_ptr(rcx, offsetof(::frame, pc)));
    a.add(rdx, (int32_t)sizeof(opcode_instr));
    a.mov(fiber_field(offsetof(fiber, pc)), rdx);

    a.mov(rdx, fiber_field(offsetof(fiber, stack)));
    a.mov(SP, x86::qword_ptr(rcx, offsetof(::frame, sp)));
    a.shl(SP, 4);                                       // slots -> bytes
    a.add(SP, rdx);
    a.mov(fiber_field(offsetof(fiber, sp)), SP);
    a.mov(FP, x86::qword_ptr(rcx, offsetof(::frame, fp)));
    a.shl(FP, 4);
    a.add(FP, rdx);
    a.mov(fiber_field(offsetof(fiber, fp)), FP);
    a.mov(fiber_field(offsetof(fiber, ptr)), PTR);             // the object pointer survives a return

    a.xor_(x86::eax, x86::eax);                         // JIT_OK
    a.jmp(epilogue);

    // the fiber's first function returns: return_method() marks the fiber finished
    a.bind(lastFrame);
    save_stack_registers();
    save_vm_registers();
    a.mov(rdi, THREAD);
    a.mov(x86::esi, JIT_OK);
    call_runtime(reinterpret_cast<void*>(jit_rt_leave));
    a.jmp(leave);
}

} // namespace x64jit
