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
        // the slot forms that jump: the target is the second word
        case Opcode::JLT_S: case Opcode::JGT_S: case Opcode::JLE_S: case Opcode::JGE_S:
        case Opcode::JEQ_S: case Opcode::JNE_S:
        case Opcode::DJLT_S: case Opcode::DJGT_S: case Opcode::DJLE_S: case Opcode::DJGE_S:
        case Opcode::DJEQ_S: case Opcode::DJNE_S:
        case Opcode::JNZ_S: case Opcode::JZ_S: case Opcode::JNULL_S:
        case Opcode::IADD_S: case Opcode::ISUB_S: case Opcode::IMUL_S:
        case Opcode::IAND_S: case Opcode::IOR_S: case Opcode::IXOR_S: case Opcode::ISHL_S: case Opcode::ISHR_S:
        case Opcode::JLT_K: case Opcode::JGT_K: case Opcode::JLE_K: case Opcode::JGE_K:
        case Opcode::JEQ_K: case Opcode::JNE_K:
        case Opcode::DJLT_K: case Opcode::DJGT_K: case Opcode::DJLE_K: case Opcode::DJGE_K:
        case Opcode::DJEQ_K: case Opcode::DJNE_K:
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
    doorEntry = m.label();
    callNotFinished = m.label();
    resumeEntry = m.label();
    resumeCheck = m.label();
    dispatch = m.label();
    exception = m.label();
    exitSwitch = m.label();
    exitKill = m.label();
    leave = m.label();
    decline = m.label();

    pick_local_homes();
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

/**
 * The local numbers for the machine to keep in CPU registers (ArmMachine::home_locals), picked as
 * the x64 JIT picks them (X64Generator::pick_slot_homes): every use of a slot as a number is
 * counted, a use inside a loop eight times, inside two loops sixty-four times (a loop is what a
 * backward jump encloses); the four heaviest that are mostly whole numbers and the four heaviest
 * that are mostly decimals are named, the heaviest first.
 */
void ArmGenerator::pick_local_homes() {
    const int most = 4;         // of each kind

    // how deep in loops each instruction is
    std::vector<uint8_t> depth((size_t) count, 0);
    for(Int at = 0; at < count; at += instruction_length(GET_OP(code[at]))) {
        const uint32_t word = code[at];
        const int o = GET_OP(word);
        Int target = -1;
        if(o == Opcode::JMP || o == Opcode::JE || o == Opcode::JNE) target = GET_Da(word);
        else if(((o >= Opcode::JLT_S && o <= Opcode::DJNE_S) || o == Opcode::JNZ_S || o == Opcode::JZ_S || o == Opcode::JNULL_S
                 || (o >= Opcode::JLT_K && o <= Opcode::DJNE_K))
                && at + 1 < count) target = (int32_t) code[at + 1];
        if(target >= 0 && target <= at)
            for(Int q = target; q <= at; q++) if(depth[(size_t) q] < 3) depth[(size_t) q]++;
    }

    struct weight { double whole = 0, decimal = 0, either = 0; };
    std::vector<weight> weights;
    double now = 1;
    double reloads = 0;         // how often the homes are read again: after every call out of the function
    auto use = [&](Int slot, int kind) {         // kind 0: either, 1: whole, 2: decimal
        if(slot < 0 || slot > 1000) return;
        if((size_t) slot >= weights.size()) weights.resize((size_t) slot + 1);
        if(kind == 1) weights[(size_t) slot].whole += now;
        else if(kind == 2) weights[(size_t) slot].decimal += now;
        else weights[(size_t) slot].either += now;
    };
    for(Int at = 0; at < count; at += instruction_length(GET_OP(code[at]))) {
        const uint32_t word = code[at];
        const int o = GET_OP(word);
        now = (double) (1 << (3 * depth[(size_t) at]));
        switch(o) {
            // (after each of these fp is loaded again, and the homes with it)
            case Opcode::CALL: case Opcode::CALLD: case Opcode::INVOKE_DELEGATE: case Opcode::INT:
            case Opcode::NEWCLASS: case Opcode::NEWARRAY: case Opcode::NEWOBJARRAY: case Opcode::NEWCLASSARRAY:
            case Opcode::NEWSTRING: case Opcode::CAST: case Opcode::VARCAST: case Opcode::IS:
            case Opcode::MOD: case Opcode::IMOD:
                reloads += now;
                break;
            // a number written through sp or the fiber's stack at a place the instruction names
            // could be one of our locals: no homes (arm_machine.h, what home_locals is promised)
            case Opcode::SMOVR: case Opcode::ISADD: case Opcode::DISADD: case Opcode::MOV_ABS:
                return;
            case Opcode::ADD_S: case Opcode::SUB_S: case Opcode::MUL_S: case Opcode::AND_S:
            case Opcode::OR_S: case Opcode::XOR_S: case Opcode::SHL_S: case Opcode::SHR_S:
                use(GET_Ba(word), 1); use(GET_Bb(word), 1); use(GET_Bc(word), 1);
                break;
            case Opcode::DADD_S: case Opcode::DSUB_S: case Opcode::DMUL_S: case Opcode::DDIV_S:
                use(GET_Ba(word), 2); use(GET_Bb(word), 2); use(GET_Bc(word), 2);
                break;
            case Opcode::I2D_S: use(GET_Ba(word), 2); use(GET_Bb(word), 1); break;
            case Opcode::D2I_S: use(GET_Ba(word), 1); use(GET_Bb(word), 2); break;
            case Opcode::JLT_S: case Opcode::JGT_S: case Opcode::JLE_S: case Opcode::JGE_S:
            case Opcode::JEQ_S: case Opcode::JNE_S:
                use(GET_Ba(word), 1); use(GET_Bb(word), 1);
                break;
            case Opcode::DJLT_S: case Opcode::DJGT_S: case Opcode::DJLE_S: case Opcode::DJGE_S:
            case Opcode::DJEQ_S: case Opcode::DJNE_S:
                use(GET_Ba(word), 2); use(GET_Bb(word), 2);
                break;
            case Opcode::ALOAD8_S: case Opcode::ALOADU8_S: case Opcode::ALOAD16_S: case Opcode::ALOADU16_S:
            case Opcode::ALOAD32_S: case Opcode::ALOADU32_S: case Opcode::ALOAD64_S:
                use(GET_Ba(word), 0); use(GET_Bc(word), 1);
                break;
            case Opcode::ASTORE8_S: case Opcode::ASTORE16_S: case Opcode::ASTORE32_S: case Opcode::ASTORE64_S:
                use(GET_Bb(word), 1); use(GET_Bc(word), 0);
                break;
            case Opcode::GETF_S: use(GET_Ba(word), 0); break;
            case Opcode::PUTF_S: use(GET_Bc(word), 0); break;
            case Opcode::SIZEOF_S: use(GET_Ba(word), 1); break;
            case Opcode::AGETOBJ_S: use(GET_Bc(word), 1); break;
            case Opcode::JNZ_S: case Opcode::JZ_S: use(GET_Da(word), 1); break;
            case Opcode::IAND_S: case Opcode::IOR_S: case Opcode::IXOR_S: case Opcode::ISHL_S: case Opcode::ISHR_S:
                use(GET_Ba(word), 1); use(GET_Bb(word), 1);
                break;
            case Opcode::JLT_K: case Opcode::JGT_K: case Opcode::JLE_K: case Opcode::JGE_K:
            case Opcode::JEQ_K: case Opcode::JNE_K:
                use(GET_Ba(word), 1);
                break;
            case Opcode::DJLT_K: case Opcode::DJGT_K: case Opcode::DJLE_K: case Opcode::DJGE_K:
            case Opcode::DJEQ_K: case Opcode::DJNE_K:
                use(GET_Ba(word), 2);
                break;
            case Opcode::ISNULL_S: use(GET_Ba(word), 1); break;
            case Opcode::NOT_S: case Opcode::MOV8_S: case Opcode::MOV16_S: case Opcode::MOV32_S:
            case Opcode::MOVU8_S: case Opcode::MOVU16_S: case Opcode::MOVU32_S:
            case Opcode::IADD_S: case Opcode::ISUB_S: case Opcode::IMUL_S:
                use(GET_Ba(word), 1); use(GET_Bb(word), 1);
                break;
            case Opcode::RETURNVAL_S: use(GET_Da(word), 0); break;
            case Opcode::LOADL: case Opcode::SMOVR_2: use(GET_Cb(word), 0); break;
            case Opcode::SMOVR_4: use(GET_Da(word), 0); if(at + 1 < count) use((int32_t) code[at + 1], 0); break;
            case Opcode::IPUSHL: case Opcode::IPOPL: use(GET_Da(word), 0); break;
            case Opcode::ISTOREL: case Opcode::IADDL: case Opcode::ISUBL: case Opcode::IMULL: case Opcode::IMODL:
                use(GET_Da(word), 1);
                break;
            case Opcode::DISTOREL: case Opcode::DIADDL: case Opcode::DISUBL: case Opcode::DIMULL: case Opcode::DIDIVL:
                use(GET_Da(word), 2);
                break;
            case Opcode::ADDL: case Opcode::SUBL: case Opcode::MULL: case Opcode::MODL:
            case Opcode::ANDL: case Opcode::ORL: case Opcode::XORL:
                use(GET_Cb(word), 1);
                break;
            case Opcode::DADDL: case Opcode::DSUBL: case Opcode::DMULL: case Opcode::DDIVL:
                use(GET_Cb(word), 2);
                break;
            default:
                break;
        }
    }

    std::vector<int32_t> whole, decimal;
    for(int round = 0; round < 2 * most; round++) {
        int best = -1;
        // a home saves a read per use and costs one per reload: it must be used clearly more often than
        // it is reloaded (and a slot named once gains nothing)
        double bestTotal = 1.5 + 0.5 * reloads;
        for(size_t slot = 0; slot < weights.size(); slot++) {
            const weight &w = weights[slot];
            double total = w.whole + w.decimal + w.either;
            bool asDecimal = w.decimal > w.whole;
            if((asDecimal ? decimal.size() : whole.size()) >= (size_t) most) continue;
            if(total > bestTotal) { bestTotal = total; best = (int) slot; }
        }
        if(best < 0) break;
        weight &w = weights[(size_t) best];
        if(w.decimal > w.whole) decimal.push_back(slot_number(best));
        else whole.push_back(slot_number(best));
        w.whole = w.decimal = w.either = 0;
    }
    m.home_locals(whole, decimal);
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
        &ArmGenerator::emit_console,
        &ArmGenerator::emit_slot_forms
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
    path.data = nullptr;
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
    m.slot_operation_constant(how, slot_number(local), slot_number(local), value);
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
 * A jump that compares and jumps in one (the slot forms) calls this first, then compares, then
 * jumps straight to labels[target]. For a jump BACKWARDS - the bottom of a loop - it is the safe
 * point: the thread looks at its signals here, before the comparison, and comes back to THIS
 * instruction when it has dealt with them. So the loop's own path is: one look at the signal
 * word, the comparison, one jump taken.   false: `target` is not an instruction.
 */
bool ArmGenerator::before_conditional_jump(Int target) {
    if(target < 0 || target >= count || !starts[target]) return false;
    if(target <= pc) jump_if_signal(request_slow_path(slow_safepoint, pc));
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
