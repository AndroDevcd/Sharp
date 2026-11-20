//
// The ARM generator's own part: walking the bytecode, decoding an instruction, and the building
// blocks the sections/ and opcode/ files are written with. arm_generator.h has the map.
//
#include "arm_generator.h"
#include <cstring>

static_assert(sizeof(stack_item) == 16, "arm jit: a stack slot is 16 bytes");
static_assert(sizeof(object) == 8, "arm jit: an object slot is 8 bytes");
static_assert(sizeof(vm_cell) == 8, "arm jit: a VM number is 8 bytes");

namespace armjit {

int instruction_length(int op) {
    switch(op) {
        case Opcode::MOVI: case Opcode::IADD: case Opcode::ISUB: case Opcode::IMUL: case Opcode::DIDIV:
        case Opcode::IMOD: case Opcode::NEWCLASS: case Opcode::MOVN: case Opcode::NEWCLASSARRAY:
        case Opcode::IADDL: case Opcode::ISUBL: case Opcode::IMULL: case Opcode::DIDIVL: case Opcode::IMODL:
        case Opcode::ISTORE: case Opcode::ISTOREL: case Opcode::SMOVR_4: case Opcode::ISADD: case Opcode::CMP:
        case Opcode::IS: case Opcode::INVOKE_DELEGATE:
        case Opcode::DIADD: case Opcode::DISUB: case Opcode::DIMUL: case Opcode::DIADDL: case Opcode::DISUBL:
        case Opcode::DIMULL: case Opcode::DISADD: case Opcode::DISTORE: case Opcode::DISTOREL:
        case Opcode::DMOVI: case Opcode::DCMP:
            return 2;
        default:
            return 1;
    }
}

int64_t bits_of(double value) {
    int64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

ArmGenerator::ArmGenerator(ArmMachine &machine, sharp_function *fn, jit_compiled_function *out)
    : count(fn->bytecodeSize), m(machine), function(fn), record(out), code(fn->bytecode) {}

// ---------------------------------------------------------------------------------
// The whole function
// ---------------------------------------------------------------------------------

/**
 * The layout of a compiled function (the same as on x86-64):
 *
 *      entry                       sections/entry.cpp
 *      instruction 0               opcode/...
 *      instruction 1
 *      ...
 *      slow paths                  sections/slow_paths.cpp
 *      resume check + dispatch     sections/dispatch.cpp
 *      exception                   sections/exception.cpp
 *      exits                       sections/exits.cpp
 */
bool ArmGenerator::generate() {
    starts.assign(count, false);
    for(Int at = 0; at < count; at += instruction_length(GET_OP(code[at])))
        starts[at] = true;

    labels.resize(count);
    for(Int at = 0; at < count; at++)
        labels[at] = m.label();

    directEntry = m.label();
    resumeEntry = m.label();
    resumeCheck = m.label();
    dispatch = m.label();
    exception = m.label();
    exitSwitch = m.label();
    exitKill = m.label();
    leave = m.label();
    decline = m.label();

    emit_entry();

    for(pc = 0; pc < count; pc = next) {
        decode();
        m.bind(labels[pc]);
        if(!emit_instruction())
            return false;
    }

    // running off the end of the bytecode cannot happen in a well formed function (it ends in
    // RET); if it does, hand back to the interpreter
    m.jump(decline);

    emit_slow_paths();
    emit_dispatch();
    emit_exception();
    emit_exits();
    return !m.failed();
}

/** take the instruction at `pc` apart (the interpreter's single_arg, dual_arg1 ... macros) */
void ArmGenerator::decode() {
    const uint32_t word = code[pc];
    op = GET_OP(word);
    single = GET_Da(word);
    dual1 = GET_Ca(word);
    dual2 = GET_Cb(word);
    triple1 = GET_Ba(word);
    triple2 = GET_Bb(word);
    triple3 = GET_Bc(word);
    raw = pc + 1 < count ? (int32_t)code[pc + 1] : 0;
    next = pc + instruction_length(op);
}

/** ask each kind in turn; an instruction nobody knows blocks the function */
bool ArmGenerator::emit_instruction() {
    emit_result (ArmGenerator::*kinds[])() = {
        &ArmGenerator::emit_integer_math,
        &ArmGenerator::emit_decimal_math,
        &ArmGenerator::emit_stack_and_locals,
        &ArmGenerator::emit_objects,
        &ArmGenerator::emit_fields_and_arrays,
        &ArmGenerator::emit_allocation,
        &ArmGenerator::emit_casts_and_locks,
        &ArmGenerator::emit_control_flow,
        &ArmGenerator::emit_calls,
        &ArmGenerator::emit_console
    };

    for(auto kind : kinds) {
        emit_result result = (this->*kind)();
        if(result == emitted) return true;
        if(result == cannot_compile) break;
    }

    // HLT, anything unknown
    blockedBy = op;
    return false;
}

// ---------------------------------------------------------------------------------
// The fiber's copies of fp / sp / ptr, and where it is
// ---------------------------------------------------------------------------------

void ArmGenerator::save_stack_registers() {
    m.p_store(SP, TASK, (int32_t) offsetof(fiber, sp));
    m.p_store(PTR, TASK, (int32_t) offsetof(fiber, ptr));
}

void ArmGenerator::load_stack_registers() {
    m.p_load(FP, TASK, (int32_t) offsetof(fiber, fp));
    m.p_load(SP, TASK, (int32_t) offsetof(fiber, sp));
    m.p_load(PTR, TASK, (int32_t) offsetof(fiber, ptr));
}

void ArmGenerator::set_current_instruction(Int at) {
    m.p_constant(PA, (uintptr_t) (code + at));
    m.p_store(PA, TASK, (int32_t) offsetof(fiber, pc));
}

// ---------------------------------------------------------------------------------
// Calling C++
//
//      prepare_runtime_call();                                 // the fiber is current
//      call_checked(jit_rt_..., { Arg::constant(n), ... });    // (or m.call())
//      m.result_to_value(V0);                                  // the result
//
// A call destroys every scratch register: what is needed after it is loaded again.
// ---------------------------------------------------------------------------------

void ArmGenerator::prepare_runtime_call() {         // jit_runtime.h, rule 2
    set_current_instruction(pc);
    save_stack_registers();
}

void ArmGenerator::jump_if_signal(int label) {
    m.p_load_32(PC, THREAD, (int32_t) offsetof(sharp_thread, signal));      // (its low half: every bit is there)
    m.p_jump_if_not_zero(PC, label);
}

void ArmGenerator::jump_if_exception(int label) {
    m.p_load_32(PC, THREAD, (int32_t) offsetof(sharp_thread, signal));      // (its low half: the bit is there)
    m.p_jump_if_bits(PC, 1u << tsig_except, true, label);     // (hasSignal: the flag's NUMBER is the bit)
}

void ArmGenerator::call_checked(void *target, const std::vector<Arg> &arguments) {
    m.call(target, arguments);
    // (the result is still where the call left it: only PC is used to look at the signal)
    jump_if_exception(exception);
}

// ---------------------------------------------------------------------------------
// The stack
// ---------------------------------------------------------------------------------

int ArmGenerator::request_slow_path(slow_kind kind, Int at, int back) {
    slow_path_request path;
    path.kind = kind;
    path.entry = m.label();
    path.back = back;
    path.pc = at;
    slowPaths.push_back(path);
    return path.entry;
}

/**
 * (++sp): room for one more slot, like grow_stack + stack_overflow_check.
 * task->growAt is the last slot that can be filled without growing.
 */
void ArmGenerator::push_stack_slot() {
    int back = m.label();
    m.p_load(PC, TASK, (int32_t) offsetof(fiber, growAt));
    m.p_jump_if(SP, PC, P_HIGHER_OR_SAME, request_slow_path(slow_grow, pc, back));
    m.bind(back);
    m.p_add(SP, SP, 16);
}

void ArmGenerator::pop_stack_slot() {
    m.p_add(SP, SP, -16);
}

// ---------------------------------------------------------------------------------
// The object pointer
// ---------------------------------------------------------------------------------

/** to = ptr->o, or a nullptr_exception (the interpreter's require_object_with_value) */
void ArmGenerator::load_object_or_throw(PReg to) {
    int null = request_slow_path(slow_null, pc);
    m.p_jump_if_zero(PTR, null);
    m.p_load(to, PTR, 0);
    m.p_jump_if_zero(to, null);
}

/** to = ptr ? ptr->o : null   (what copy_object(to, ptr) stores) */
void ArmGenerator::load_object_or_null(PReg to) {
    int done = m.label();
    m.p_constant(to, 0);
    m.p_jump_if_zero(PTR, done);
    m.p_load(to, PTR, 0);
    m.bind(done);
}

// ---------------------------------------------------------------------------------
// Whole numbers
// ---------------------------------------------------------------------------------

void ArmGenerator::integer_operation(VOp how, Int left, Int right, Int result) {
    load_register(V0, left);
    load_register(V1, right);
    m.v_operation(how, V0, V0, V1);
    store_register(result, V0);
}

void ArmGenerator::integer_operation_with_constant(VOp how, Int n, int64_t value) {
    load_register(V0, n);
    m.v_constant(V1, value);
    m.v_operation(how, V0, V0, V1);
    store_register(n, V0);
}

void ArmGenerator::integer_compare(Int left, Int right, Int out, Cond how) {
    load_register(V0, left);
    load_register(V1, right);
    m.v_compare(V0, V0, V1, how);
    store_register(out, V0);
}

void ArmGenerator::local_operation(VOp how, Int local, Int n) {
    m.v_load(V0, FP, slot_number(local));
    load_register(V1, n);
    m.v_operation(how, V0, V0, V1);
    m.v_store(V0, FP, slot_number(local));
}

void ArmGenerator::local_operation_with_constant(VOp how, Int local, int64_t value) {
    m.v_load(V0, FP, slot_number(local));
    m.v_constant(V1, value);
    m.v_operation(how, V0, V0, V1);
    m.v_store(V0, FP, slot_number(local));
}

/** V0 = vm_remainder(V0, V1): the runtime does it (it raises on a zero divisor) */
void ArmGenerator::remainder_into_v0() {
    prepare_runtime_call();
    call_checked(reinterpret_cast<void*>(jit_rt_remainder), { Arg::value(V0), Arg::value(V1) });
    m.result_to_value(V0);
}

// ---------------------------------------------------------------------------------
// Decimals
// ---------------------------------------------------------------------------------

void ArmGenerator::decimal_operation(DOp how, Int left, Int right, Int result) {
    load_decimal(D0, left);
    load_decimal(D1, right);
    m.d_operation(how, D0, D0, D1);
    store_decimal(result, D0);
}

void ArmGenerator::decimal_operation_with_constant(DOp how, Int n, double value) {
    load_decimal(D0, n);
    m.d_constant(D1, value);
    m.d_operation(how, D0, D0, D1);
    store_decimal(n, D0);
}

void ArmGenerator::decimal_compare(Int left, Int right, Int out, Cond how) {
    load_decimal(D0, left);
    load_decimal(D1, right);
    m.d_compare(V0, D0, D1, how);
    store_register(out, V0);
}

void ArmGenerator::local_decimal_operation(DOp how, Int local, Int n) {
    m.d_load(D0, FP, slot_number(local));
    load_decimal(D1, n);
    m.d_operation(how, D0, D0, D1);
    m.d_store(D0, FP, slot_number(local));
}

void ArmGenerator::local_decimal_operation_with_constant(DOp how, Int local, double value) {
    m.d_load(D0, FP, slot_number(local));
    m.d_constant(D1, value);
    m.d_operation(how, D0, D0, D1);
    m.d_store(D0, FP, slot_number(local));
}

// ---------------------------------------------------------------------------------
// Jumps and calls
// ---------------------------------------------------------------------------------

/** jump to the instruction at `target`; false when there is none */
bool ArmGenerator::jump_to_instruction(Int target) {
    if(target < 0 || target >= count || !starts[target]) return false;

    // a jump backwards is the bottom of a loop: the place a running thread looks at its signals
    // (the interpreter's check_state after a jump)
    if(target <= pc) jump_if_signal(request_slow_path(slow_safepoint, target));
    m.jump(labels[target]);
    return true;
}

/**
 * After a helper that ran other Sharp code (jit_rt_call ...). The callee has already moved task->pc.
 *      if(task->current != this function) return JIT_CONTEXT_SWITCH;   // it did not finish
 *      if(thread->signal) -> the resume check (exception, safe point, then dispatch)
 * continuesHere = false: the code after the call is wherever task->pc points (invoke_delegate),
 * not the next instruction.
 */
void ArmGenerator::check_after_runtime_call(bool continuesHere) {
    m.p_load(PA, TASK, (int32_t) offsetof(fiber, current));
    m.p_constant(PB, (uintptr_t) function);
    m.p_jump_if(PA, PB, P_NE, exitSwitch);
    load_stack_registers();
    if(continuesHere) jump_if_signal(resumeCheck);
    else m.jump(resumeCheck);
}

} // namespace armjit
