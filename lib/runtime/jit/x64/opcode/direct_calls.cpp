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
#include "../../../memory/function_value.h"

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
 *      if(!callee->isHighFrequency || no room on the stack || no room in frames
 *              || the thread's machine stack is nearly used up)
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
 * Nothing is passed: thread, task, sp, fp, ptr and the VM registers are in the
 * CPU registers both functions agree on. Only rax, rcx and rdx are used as
 * scratch here - rsi and rdi are homes of VM registers. What IS kept, in our
 * machine frame, is how far our fp is below sp: with it we put our own fp
 * back when the callee returns ("the short way back", below).
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

    // if(rsp < thread->nativeStackFloor) -> the slow way, which raises
    // stack_overflow_exception: the call below is a real machine call, and the
    // thread's machine stack is nearly used up (sharp_thread.nativeStackFloor)
    a.cmp(x86::rsp, x86::qword_ptr(THREAD, offsetof(sharp_thread, nativeStackFloor)));
    a.jb(slowCall);

    // ---- record how to get back
    a.mov(rax, SP);                                     // (for us: see "the short way back" below)
    a.sub(rax, FP);
    a.mov(machine_frame_slot(FRAME_FP_BELOW_SP), rax);
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

    // Anything but JIT_OK: sections/dispatch.cpp sorts it out (the fiber holds the state).
    a.test(x86::eax, x86::eax);
    a.jnz(callNotFinished);

    // THE SHORT WAY BACK. The callee was entered by this direct call, so its RET only took its
    // frame off the VM's call stack (direct_return); what return_method() would restore, we know
    // without looking anything up:
    //      sp before the call = the CALLEE's fp + callee->fpOffset   (fp still holds the callee's)
    //      fp = that sp - the distance kept in our machine frame     (a distance, not an address:
    //                                                                 the callee may have grown the stack, which moves it)
    //      sp = where the frame record says: (sp before the call) - callee->spOffset
    //      task->current / rom / pc = this function, its code, the instruction after this CALL
    a.lea(SP, x86::qword_ptr(FP, (int32_t)(callee->fpOffset * 16)));
    a.mov(FP, SP);
    a.sub(FP, machine_frame_slot(FRAME_FP_BELOW_SP));
    if(callee->spOffset != 0)
        a.lea(SP, x86::qword_ptr(SP, (int32_t)(-callee->spOffset * 16)));
    a.mov(fiber_field(offsetof(fiber, sp)), SP);
    a.mov(fiber_field(offsetof(fiber, fp)), FP);
    a.mov(rax, (uint64_t)function);
    a.mov(fiber_field(offsetof(fiber, current)), rax);
    a.mov(rax, (uint64_t)code);
    a.mov(fiber_field(offsetof(fiber, rom)), rax);
    a.mov(rax, (uint64_t)(code + pc + 1));
    a.mov(fiber_field(offsetof(fiber, pc)), rax);

    a.cmp(thread_signal(), 0);                                 // check_state after a call
    a.jne(resumeFromRegisters);

    a.bind(afterCall);                                  // the slow way joins here
    load_slot_homes();                                  // the callee had its own slots in those registers
}

/**
 * INVOKE_DELEGATE address, count   (not static: a call of an overridden function)
 *
 * Which function runs is the receiver's class's affair - the receiver is the object under the
 * arguments. invoke_delegate() searches the class and its bases for it on every call; here each
 * place has a cache of the classes it has met (jit_runtime.h), and a class found in it is called
 * exactly as direct_call calls: the same steps, with the callee's numbers read from its record
 * instead of written into the code, because the callee is only known when the code runs.
 *
 *      class = receiver->info & CLASS_MASK
 *      for each entry of the cache: if(entry.class == class) { callee = vm.methods + entry.function; goto call; }
 *      -> the slow way (it looks the function up, puts it into the cache, calls it)
 */
void X64Generator::virtual_call(Int argumentCount) {
    uint64_t *cache = jit_new_virtual_call_cache();
    Label afterCall = a.newLabel(), found = a.newLabel();
    Label slowCall = request_slow_path(slow_virtual_call, pc, afterCall);
    slowPaths.back().data = cache;

    // ---- which function
    a.mov(rax, stack_object(-argumentCount));           // the receiver
    a.test(rax, rax);
    a.jz(slowCall);                                     // null: the runtime raises
    a.mov(x86::ecx, x86::dword_ptr(rax, offsetof(sharp_object, info)));
    a.and_(x86::ecx, CLASS_MASK);
    a.mov(rax, (uint64_t)cache);
    for(int i = 0; i < VIRTUAL_CALL_CACHE_ENTRIES; i++) {
        a.mov(rdx, x86::qword_ptr(rax, i * 8));
        a.cmp(x86::edx, x86::ecx);                      // the low half: the class
        a.je(found);
    }
    a.jmp(slowCall);
    a.bind(found);
    a.shr(rdx, 32);                                     // the high half: the function's address
    a.imul(rdx, rdx, (int32_t)sizeof(sharp_function));
    a.mov(rax, (uint64_t)vm.methods);
    a.add(rax, rdx);                                    // rax = the callee

    call_function_in_rax(2, slowCall, afterCall);       // (invoke_delegate is two words)
}

/**
 * CALLD count      (a call of a function value: a lambda, a function passed along)
 *
 *      stack:   [ function value ][ argument 1 ] ... [ argument n ]   <- sp
 *
 * call_function_value() (memory/function_value.cpp), written out: the arguments move down over
 * the function value, what it captured - if anything - becomes one more argument, and the
 * function whose address it holds is called like any other.
 */
void X64Generator::function_value_call(Int argumentCount) {
    Label afterCall = a.newLabel(), noCapture = a.newLabel(), ready = a.newLabel();
    Label raise = request_slow_path(slow_function_value_call, pc, afterCall);
    Label slowCall = request_slow_path(slow_found_call, pc, afterCall);
    const int32_t valueSlot = (int32_t)(-argumentCount * 16);       // from sp: where the function value sits

    // ---- the function value's two parts: rax = the address, rcx = what it captured
    a.mov(rax, x86::qword_ptr(SP, valueSlot + 8));
    a.test(rax, rax);
    a.jz(raise);                                        // null
    a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
    a.mov(rax, x86::qword_ptr(rcx, (int32_t)(function_value_address_slot() * 8)));
    a.mov(rcx, x86::qword_ptr(rcx, (int32_t)(function_value_captured_slot() * 8)));
    a.test(rax, rax);
    a.jle(raise);                                       // no function has address 0 or less ...
    a.cmp(rax, (int32_t)vm.mf.methods);
    a.jge(raise);                                       // ... or one past the last

    // ---- the arguments close the gap the function value leaves (a slot is a number and an object)
    for(Int i = 0; i < argumentCount; i++) {
        a.mov(rdx, x86::qword_ptr(SP, valueSlot + (int32_t)((i + 1) * 16)));
        a.mov(x86::qword_ptr(SP, valueSlot + (int32_t)(i * 16)), rdx);
        a.mov(rdx, x86::qword_ptr(SP, valueSlot + (int32_t)((i + 1) * 16) + 8));
        a.mov(x86::qword_ptr(SP, valueSlot + (int32_t)(i * 16) + 8), rdx);
    }
    // the top slot is free now: what was captured goes there (the hidden last parameter), or the slot goes
    a.test(rcx, rcx);
    a.jz(noCapture);
    a.mov(stack_object(), rcx);
    a.jmp(ready);
    a.bind(noCapture);
    a.mov(stack_object(), 0);
    a.sub(SP, 16);
    a.bind(ready);

    a.imul(rax, rax, (int32_t)sizeof(sharp_function));
    a.mov(rdx, (uint64_t)vm.methods);
    a.add(rax, rdx);                                    // rax = the callee

    call_function_in_rax(1, slowCall, afterCall);
}

/**
 * Call the function whose record rax points at - one found while the code ran (virtual_call,
 * function_value_call). The steps of direct_call, with the callee's numbers read from its record
 * instead of written into the code. `instructionWords`: how long the calling instruction is (the
 * VM keeps the pc of its last word in the frame, and continues after it).
 */
void X64Generator::call_function_in_rax(Int instructionWords, const Label &slowCall, const Label &afterCall) {
    a.mov(machine_frame_slot(FRAME_CALLEE), rax);

    // ---- may it be called directly? (the tests of direct_call)
    a.cmp(x86::byte_ptr(rax, offsetof(sharp_function, isHighFrequency)), 0);
    a.je(slowCall);
    a.movsxd(rcx, x86::dword_ptr(rax, offsetof(sharp_function, stackSize)));
    a.shl(rcx, 4);
    a.lea(rcx, x86::qword_ptr(SP, rcx, 0, -16));
    a.cmp(rcx, fiber_field(offsetof(fiber, growAt)));
    a.jae(slowCall);
    a.mov(rcx, fiber_field(offsetof(fiber, callFramePtr)));
    a.inc(rcx);
    a.cmp(rcx, fiber_field(offsetof(fiber, frameSize)));
    a.jae(slowCall);
    a.cmp(x86::rsp, x86::qword_ptr(THREAD, offsetof(sharp_thread, nativeStackFloor)));
    a.jb(slowCall);

    // ---- record how to get back (prepare_method's frame)
    a.mov(rdx, SP);
    a.sub(rdx, FP);
    a.mov(machine_frame_slot(FRAME_FP_BELOW_SP), rdx);
    a.inc(fiber_field(offsetof(fiber, calls)));
    a.mov(fiber_field(offsetof(fiber, callFramePtr)), rcx);
    frame_address(a);
    a.mov(x86::dword_ptr(rcx, offsetof(::frame, returnAddress)), (int32_t)function->address);
    a.mov(rdx, (uint64_t)(code + pc + instructionWords - 1));
    a.mov(x86::qword_ptr(rcx, offsetof(::frame, pc)), rdx);
    a.movsxd(rdx, x86::dword_ptr(rax, offsetof(sharp_function, spOffset)));
    a.shl(rdx, 4);
    a.neg(rdx);
    a.add(rdx, SP);                                     // sp - callee->spOffset
    a.sub(rdx, fiber_field(offsetof(fiber, stack)));
    a.sar(rdx, 4);                                      // bytes -> slots
    a.mov(x86::qword_ptr(rcx, offsetof(::frame, sp)), rdx);
    a.mov(rdx, FP);
    a.sub(rdx, fiber_field(offsetof(fiber, stack)));
    a.sar(rdx, 4);
    a.mov(x86::qword_ptr(rcx, offsetof(::frame, fp)), rdx);
    a.mov(x86::byte_ptr(rcx, offsetof(::frame, native)), 1);

    // ---- the callee's frame
    a.mov(fiber_field(offsetof(fiber, current)), rax);
    a.movsxd(rdx, x86::dword_ptr(rax, offsetof(sharp_function, fpOffset)));
    a.shl(rdx, 4);
    a.mov(FP, SP);
    a.sub(FP, rdx);                                     // fp = sp - callee->fpOffset
    a.mov(fiber_field(offsetof(fiber, fp)), FP);
    a.movsxd(rdx, x86::dword_ptr(rax, offsetof(sharp_function, frameStackOffset)));
    a.shl(rdx, 4);
    a.add(SP, rdx);
    a.mov(rdx, x86::qword_ptr(rax, offsetof(sharp_function, bytecode)));
    a.mov(fiber_field(offsetof(fiber, rom)), rdx);
    a.mov(fiber_field(offsetof(fiber, pc)), rdx);

    // ---- run it
    a.mov(rax, x86::qword_ptr(rax, offsetof(sharp_function, jfunc)));
    a.call(x86::qword_ptr(rax, offsetof(jit_compiled_function, directEntry)));
    a.test(x86::eax, x86::eax);
    a.jnz(callNotFinished);

    // ---- the short way back (as in direct_call; the callee's numbers come from its record)
    a.mov(rax, machine_frame_slot(FRAME_CALLEE));
    a.movsxd(rdx, x86::dword_ptr(rax, offsetof(sharp_function, fpOffset)));
    a.shl(rdx, 4);
    a.lea(SP, x86::qword_ptr(FP, rdx));                 // sp before the call
    a.mov(FP, SP);
    a.sub(FP, machine_frame_slot(FRAME_FP_BELOW_SP));
    a.movsxd(rdx, x86::dword_ptr(rax, offsetof(sharp_function, spOffset)));
    a.shl(rdx, 4);
    a.sub(SP, rdx);
    a.mov(fiber_field(offsetof(fiber, sp)), SP);
    a.mov(fiber_field(offsetof(fiber, fp)), FP);
    a.mov(rax, (uint64_t)function);
    a.mov(fiber_field(offsetof(fiber, current)), rax);
    a.mov(rax, (uint64_t)code);
    a.mov(fiber_field(offsetof(fiber, rom)), rax);
    a.mov(rax, (uint64_t)(code + pc + instructionWords));
    a.mov(fiber_field(offsetof(fiber, pc)), rax);

    a.cmp(thread_signal(), 0);                                 // check_state after a call
    a.jne(resumeFromRegisters);

    a.bind(afterCall);
    load_slot_homes();
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
    Label theLongWay = a.newLabel();

    // Entered by a direct call (sections/entry.cpp)? Then only the frame comes off the VM's call
    // stack; the caller restores the rest itself ("the short way back" in direct_call).
    a.cmp(machine_frame_slot(FRAME_DIRECT), 0);
    a.je(theLongWay);
    a.dec(fiber_field(offsetof(fiber, calls)));
    a.dec(fiber_field(offsetof(fiber, callFramePtr)));
    a.mov(fiber_field(offsetof(fiber, ptr)), PTR);             // the object pointer survives a return
    a.xor_(x86::eax, x86::eax);                         // JIT_OK
    a.jmp(epilogue);

    a.bind(theLongWay);
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
