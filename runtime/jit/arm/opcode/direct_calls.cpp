//
// Direct calls: compiled code calling compiled code without going through C++
// (jit/x64/opcode/direct_calls.cpp - the same steps, in the machine's operations).
//
// THE SLOW WAY (still used when the callee is not compiled yet)
//      machine code -> jit_rt_call() -> prepare_method() -> the callee's machine code
//      ... RET -> jit_rt_leave() -> return_method() -> back out through all of them
//
// THE DIRECT WAY
// The caller's machine code does what prepare_method() does - record how to get back (a `frame`
// in task->frames), point fp / sp / pc at the callee - and then calls the callee's machine code
// itself, through its direct door. The callee's RET takes its frame off the VM's call stack, and
// the caller puts its own fp / sp / pc back: it knows them without looking anything up.
//
// The VM's own call stack (task->frames, task->current, the stack slots) ends up EXACTLY as the
// slow way leaves it, so stack traces, exceptions, the garbage collector and continuing a fiber
// need no change, and a direct call can be mixed freely with the slow way and the interpreter.
//
// THE BODY'S FRAME holds what must survive the call (arm_machine.h, FrameSlot): the scratch
// registers do not, and the six reserved registers are the callee's while it runs.
//
#include "../arm_generator.h"
#include "../../../memory/function_value.h"

namespace armjit {

static int32_t of_fiber(size_t offset) { return (int32_t) offset; }

/**
 * if((task->callFramePtr + 1) >= task->frameSize  ||  the thread's machine stack is nearly used up)
 *      -> the slow way (it grows the frame list; or raises stack_overflow_exception: the call
 *         below is a real machine call - sharp_thread.nativeStackFloor)
 */
void ArmGenerator::jump_unless_room_for_a_frame(int slowCall) {
    m.v_load(V0, TASK, of_fiber(offsetof(fiber, callFramePtr)));
    m.v_constant(V1, 1);
    m.v_operation(V_ADD, V0, V0, V1);
    m.v_load(V1, TASK, of_fiber(offsetof(fiber, frameSize)));
    m.v_compare(V0, V0, V1, GE);
    m.v_jump_if_not_zero(V0, slowCall);
    m.p_load(PB, THREAD, (int32_t) offsetof(sharp_thread, nativeStackFloor));
    m.p_jump_if_machine_stack_below(PB, slowCall);
}

/**
 * Record how to get back (prepare_method's frame). PA = the sp we continue with after the call
 * (sp - callee->spOffset); lastWord = the pc of the calling instruction's last word.
 *
 *      (our frame) fp below sp = sp - fp;                   // for "the short way back"
 *      task->calls++;
 *      frame *f = task->frames + ++task->callFramePtr;
 *      f->sp = PA - stack;  f->fp = fp - stack;             // in slots
 *      f->returnAddress = this function;  f->pc = the calling instruction;  f->native = true;
 */
void ArmGenerator::record_the_way_back(Int lastWord) {
    m.p_subtract(PB, SP, FP);
    m.frame_store(FRAME_FP_BELOW_SP, PB);

    m.v_load(V0, TASK, of_fiber(offsetof(fiber, calls)));
    m.v_constant(V1, 1);
    m.v_operation(V_ADD, V0, V0, V1);
    m.v_store(V0, TASK, of_fiber(offsetof(fiber, calls)));
    m.v_load(V0, TASK, of_fiber(offsetof(fiber, callFramePtr)));
    m.v_operation(V_ADD, V0, V0, V1);
    m.v_store(V0, TASK, of_fiber(offsetof(fiber, callFramePtr)));

    m.v_to_pointer(PB, V0);                                 // PB = f
    m.p_multiply(PB, (uint32_t) sizeof(::frame));
    m.p_load(PC, TASK, of_fiber(offsetof(fiber, frames)));
    m.p_add_scaled(PB, PC, PB, 0);

    m.p_load(PC, TASK, of_fiber(offsetof(fiber, stack)));
    m.p_subtract(PA, PA, PC);
    m.p_shift_right(PA, 4);                                 // bytes -> slots
    m.v_from_pointer(V0, PA);
    m.v_store(V0, PB, (int32_t) offsetof(::frame, sp));
    m.p_subtract(PA, FP, PC);
    m.p_shift_right(PA, 4);
    m.v_from_pointer(V0, PA);
    m.v_store(V0, PB, (int32_t) offsetof(::frame, fp));

    m.p_constant(PA, (uintptr_t) function->address);
    m.p_store_32(PA, PB, (int32_t) offsetof(::frame, returnAddress));
    m.p_constant(PA, (uintptr_t) (code + lastWord));
    m.p_store(PA, PB, (int32_t) offsetof(::frame, pc));
    m.p_constant(PA, 1);                                    // the caller is machine code
    m.p_store_8(PA, PB, (int32_t) offsetof(::frame, native));
}

/** PA = the callee's record: call its direct door; anything but JIT_OK is sorted out in sections/dispatch.cpp */
void ArmGenerator::run_the_callee() {
    m.p_load(PA, PA, (int32_t) offsetof(sharp_function, jfunc));
    m.p_load(PA, PA, (int32_t) offsetof(jit_compiled_function, directEntry));
    m.call_compiled(PA);
    m.result_to_pointer(PA);
    m.p_jump_if_not_zero(PA, callNotFinished);
}

/**
 * The end of "the short way back": sp and fp are ours again (in the registers).
 *      task->sp = sp;  task->fp = fp;
 *      task->current / rom / pc = this function, its code, the instruction at continueAt
 *      if(thread->signal) -> the resume check                  (check_state after a call)
 */
void ArmGenerator::continue_after_call(Int continueAt, int afterCall) {
    m.p_store(SP, TASK, of_fiber(offsetof(fiber, sp)));
    m.p_store(FP, TASK, of_fiber(offsetof(fiber, fp)));
    m.p_constant(PA, (uintptr_t) function);
    m.p_store(PA, TASK, of_fiber(offsetof(fiber, current)));
    m.p_constant(PA, (uintptr_t) code);
    m.p_store(PA, TASK, of_fiber(offsetof(fiber, rom)));
    m.p_constant(PA, (uintptr_t) (code + continueAt));
    m.p_store(PA, TASK, of_fiber(offsetof(fiber, pc)));
    jump_if_signal(resumeCheck);

    m.bind(afterCall);                      // the slow way joins here
    m.stack_numbers_changed();              // the callee had its own locals in the machine's registers
}

/**
 * CALL address        (see prepare_method() in virtual_machine.cpp)
 *
 *      if(!callee->isHighFrequency || no room on the stack || no room for a frame) -> the slow way
 *      record the way back
 *      task->current = callee;
 *      fp = sp - callee->fpOffset;   sp += callee->frameStackOffset;
 *      task->rom = task->pc = callee->bytecode;
 *      call the callee's direct door
 *
 *      THE SHORT WAY BACK: the callee's RET only took its frame off the VM's call stack.
 *      sp before the call = the CALLEE's fp + callee->fpOffset     (fp still holds the callee's)
 *      fp = that sp - the distance kept in our frame               (a distance, not an address:
 *                                                                   the callee may have grown the stack, which moves it)
 *      sp = (sp before the call) - callee->spOffset
 */
void ArmGenerator::direct_call(Int address) {
    sharp_function *callee = vm.methods + address;
    int afterCall = m.label();
    int slowCall = request_slow_path(slow_call, pc, afterCall);

    // compiled yet? (it gets compiled by prepare_method on the slow way)
    m.p_constant(PA, (uintptr_t) callee);
    m.p_load_8(PB, PA, (int32_t) offsetof(sharp_function, isHighFrequency));
    m.p_jump_if_zero(PB, slowCall);
    // grow_stack_for(callee->stackSize): sp + stackSize must stay below the end
    m.p_add(PB, SP, (int32_t) ((callee->stackSize - 1) * 16));
    m.p_load(PC, TASK, of_fiber(offsetof(fiber, growAt)));
    m.p_jump_if(PB, PC, P_HIGHER_OR_SAME, slowCall);
    jump_unless_room_for_a_frame(slowCall);

    m.p_add(PA, SP, (int32_t) (-callee->spOffset * 16));
    record_the_way_back(pc);

    // ---- the callee's frame
    m.p_constant(PA, (uintptr_t) callee);
    m.p_store(PA, TASK, of_fiber(offsetof(fiber, current)));
    m.p_add(FP, SP, (int32_t) (-callee->fpOffset * 16));
    m.p_store(FP, TASK, of_fiber(offsetof(fiber, fp)));
    if(callee->frameStackOffset != 0)
        m.p_add(SP, SP, (int32_t) (callee->frameStackOffset * 16));
    m.p_constant(PB, (uintptr_t) callee->bytecode);
    m.p_store(PB, TASK, of_fiber(offsetof(fiber, rom)));
    m.p_store(PB, TASK, of_fiber(offsetof(fiber, pc)));

    run_the_callee();

    // ---- the short way back
    m.p_add(SP, FP, (int32_t) (callee->fpOffset * 16));
    m.frame_load(PA, FRAME_FP_BELOW_SP);
    m.p_subtract(FP, SP, PA);
    if(callee->spOffset != 0)
        m.p_add(SP, SP, (int32_t) (-callee->spOffset * 16));
    continue_after_call(pc + 1, afterCall);
}

/**
 * Call the function whose record is in PA - one found while the code ran (virtual_call,
 * function_value_call). The steps of direct_call, with the callee's numbers read from its record
 * instead of written into the code. The record is kept in the body's frame (FRAME_KEPT) over the
 * call. `instructionWords`: how long the calling instruction is (the VM keeps the pc of its last
 * word in the frame, and continues after it).
 */
void ArmGenerator::call_found_function(Int instructionWords, int slowCall, int afterCall) {
    m.frame_store(FRAME_KEPT, PA);

    // ---- may it be called directly? (the tests of direct_call)
    m.p_load_8(PB, PA, (int32_t) offsetof(sharp_function, isHighFrequency));
    m.p_jump_if_zero(PB, slowCall);
    m.p_load_int(PB, PA, (int32_t) offsetof(sharp_function, stackSize));
    m.p_add_scaled(PB, SP, PB, 4);
    m.p_add(PB, PB, -16);
    m.p_load(PC, TASK, of_fiber(offsetof(fiber, growAt)));
    m.p_jump_if(PB, PC, P_HIGHER_OR_SAME, slowCall);
    jump_unless_room_for_a_frame(slowCall);

    // ---- record the way back: PA = sp - callee->spOffset
    m.frame_load(PA, FRAME_KEPT);
    m.p_load_int(PB, PA, (int32_t) offsetof(sharp_function, spOffset));
    m.p_add_scaled(PB, SP, PB, 4);
    m.p_subtract(PB, PB, SP);                               // spOffset, in bytes
    m.p_subtract(PA, SP, PB);
    record_the_way_back(pc + instructionWords - 1);

    // ---- the callee's frame
    m.frame_load(PA, FRAME_KEPT);
    m.p_store(PA, TASK, of_fiber(offsetof(fiber, current)));
    m.p_load_int(PB, PA, (int32_t) offsetof(sharp_function, fpOffset));
    m.p_add_scaled(PB, SP, PB, 4);
    m.p_subtract(PB, PB, SP);                               // fpOffset, in bytes
    m.p_subtract(FP, SP, PB);
    m.p_store(FP, TASK, of_fiber(offsetof(fiber, fp)));
    m.p_load_int(PB, PA, (int32_t) offsetof(sharp_function, frameStackOffset));
    m.p_add_scaled(SP, SP, PB, 4);
    m.p_load(PB, PA, (int32_t) offsetof(sharp_function, bytecode));
    m.p_store(PB, TASK, of_fiber(offsetof(fiber, rom)));
    m.p_store(PB, TASK, of_fiber(offsetof(fiber, pc)));

    run_the_callee();

    // ---- the short way back (as in direct_call; the callee's numbers come from its record)
    m.frame_load(PA, FRAME_KEPT);
    m.p_load_int(PB, PA, (int32_t) offsetof(sharp_function, fpOffset));
    m.p_add_scaled(SP, FP, PB, 4);                          // sp before the call
    m.frame_load(PC, FRAME_FP_BELOW_SP);
    m.p_subtract(FP, SP, PC);
    m.p_load_int(PB, PA, (int32_t) offsetof(sharp_function, spOffset));
    m.p_add_scaled(PB, SP, PB, 4);
    m.p_subtract(PB, PB, SP);
    m.p_subtract(SP, SP, PB);
    continue_after_call(pc + instructionWords, afterCall);
}

/**
 * INVOKE_DELEGATE address, count   (not static: a call of an overridden function)
 *
 * Which function runs is the receiver's class's affair - the receiver is the object under the
 * arguments. invoke_delegate() searches the class and its bases for it on every call; here each
 * place has a cache of the classes it has met (jit_runtime.h): a word is the class in its low
 * half and the function's address in its high half.
 *
 *      class = receiver->info & CLASS_MASK
 *      for each entry of the cache: if(entry.class == class) { callee = vm.methods + entry.function; goto call; }
 *      -> the slow way (it looks the function up, puts it into the cache, calls it)
 */
void ArmGenerator::virtual_call(Int argumentCount) {
    uint64_t *cache = jit_new_virtual_call_cache();
    int afterCall = m.label(), found = m.label();
    int slowCall = request_slow_path(slow_virtual_call, pc, afterCall);
    slowPaths.back().data = cache;

    // ---- which function
    m.p_load(PA, SP, slot_object(-argumentCount));          // the receiver
    m.p_jump_if_zero(PA, slowCall);                         // null: the runtime raises
    m.p_load_32(PB, PA, (int32_t) offsetof(sharp_object, info));
    m.p_and(PB, CLASS_MASK);
    m.p_constant(PC, (uintptr_t) cache);
    int entry[VIRTUAL_CALL_CACHE_ENTRIES];
    for(int i = 0; i < VIRTUAL_CALL_CACHE_ENTRIES; i++) {
        entry[i] = m.label();
        m.p_load_32(PA, PC, i * 8);                         // the low half: the class
        m.p_jump_if(PA, PB, P_EQ, entry[i]);
    }
    m.jump(slowCall);
    for(int i = 0; i < VIRTUAL_CALL_CACHE_ENTRIES; i++) {
        m.bind(entry[i]);
        m.p_load_32(PA, PC, i * 8 + 4);                     // the high half: the function's address
        m.jump(found);
    }
    m.bind(found);
    m.p_multiply(PA, (uint32_t) sizeof(sharp_function));
    m.p_constant(PB, (uintptr_t) vm.methods);
    m.p_add_scaled(PA, PB, PA, 0);                          // PA = the callee

    call_found_function(2, slowCall, afterCall);            // (invoke_delegate is two words)
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
void ArmGenerator::function_value_call(Int argumentCount) {
    int afterCall = m.label(), noCapture = m.label(), ready = m.label();
    int raise = request_slow_path(slow_function_value_call, pc, afterCall);
    int slowCall = request_slow_path(slow_found_call, pc, afterCall);
    const int32_t valueSlot = (int32_t) (-argumentCount * 16);      // from sp: where the function value sits

    // ---- the function value's two parts: V0 = the address, PC = what it captured
    m.p_load(PA, SP, valueSlot + 8);
    m.p_jump_if_zero(PA, raise);                            // null
    m.p_load(PA, PA, (int32_t) offsetof(sharp_object, node));
    m.v_load(V0, PA, (int32_t) (function_value_address_slot() * 8));
    m.p_load(PC, PA, (int32_t) (function_value_captured_slot() * 8));
    m.v_constant(V1, 0);
    m.v_compare(V1, V0, V1, LE);
    m.v_jump_if_not_zero(V1, raise);                        // no function has address 0 or less ...
    m.v_constant(V1, (int64_t) vm.mf.methods);
    m.v_compare(V1, V0, V1, GE);
    m.v_jump_if_not_zero(V1, raise);                        // ... or one past the last

    // ---- the arguments close the gap the function value leaves (a slot is a number and an object)
    for(Int i = 0; i < argumentCount; i++) {
        for(int32_t half = 0; half <= 8; half += 8) {
            m.v_load(V1, SP, valueSlot + (int32_t) ((i + 1) * 16) + half);
            m.v_store(V1, SP, valueSlot + (int32_t) (i * 16) + half);
        }
    }
    // the top slot is free now: what was captured goes there (the hidden last parameter), or the slot goes
    m.p_jump_if_zero(PC, noCapture);
    m.p_store_object(PC, SP, slot_object(0));
    m.jump(ready);
    m.bind(noCapture);
    m.p_constant(PA, 0);
    m.p_store_object(PA, SP, slot_object(0));
    m.p_add(SP, SP, -16);
    m.bind(ready);

    m.v_to_pointer(PA, V0);
    m.p_multiply(PA, (uint32_t) sizeof(sharp_function));
    m.p_constant(PB, (uintptr_t) vm.methods);
    m.p_add_scaled(PA, PB, PA, 0);                          // PA = the callee

    call_found_function(1, slowCall, afterCall);
}

/**
 * RET (without an exception)      (see return_method() and jit_rt_leave())
 *
 * Entered by a direct call (sections/entry.cpp)? Then only the frame comes off the VM's call
 * stack - the caller restores the rest itself ("the short way back" in direct_call):
 *
 *      task->calls--;  task->callFramePtr--;  task->ptr = ptr;  return JIT_OK;
 *
 * Entered by C++: the whole way, jit_rt_leave().
 */
void ArmGenerator::direct_return() {
    int theWholeWay = m.label();
    m.frame_load(PA, FRAME_DIRECT);
    m.p_jump_if_zero(PA, theWholeWay);
    m.v_constant(V1, 1);
    m.v_load(V0, TASK, of_fiber(offsetof(fiber, calls)));
    m.v_operation(V_SUB, V0, V0, V1);
    m.v_store(V0, TASK, of_fiber(offsetof(fiber, calls)));
    m.v_load(V0, TASK, of_fiber(offsetof(fiber, callFramePtr)));
    m.v_operation(V_SUB, V0, V0, V1);
    m.v_store(V0, TASK, of_fiber(offsetof(fiber, callFramePtr)));
    m.p_store(PTR, TASK, of_fiber(offsetof(fiber, ptr)));   // the object pointer survives a return
    m.return_code(JIT_OK);
    m.jump(leave);

    m.bind(theWholeWay);
    save_stack_registers();
    m.call(reinterpret_cast<void*>(jit_rt_leave), { Arg::pointer(THREAD), Arg::constant(JIT_OK) });
    m.jump(leave);          // what it returned is the return code
}

} // namespace armjit
