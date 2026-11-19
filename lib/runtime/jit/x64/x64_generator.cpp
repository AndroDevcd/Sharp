//
// The code generator's own part: walking the bytecode, decoding an instruction,
// and the building blocks the sections/ and opcode/ files are written with.
// See x64_generator.h for the map of the whole JIT and for REGISTER MODE /
// MEMORY MODE, which most of the functions here are about.
//

#include "x64_generator.h"
#include <cstring>

using namespace asmjit;

static_assert(sizeof(stack_item) == 16, "x64 jit: a stack slot is 16 bytes");
static_assert(sizeof(object) == 8, "x64 jit: an object slot is 8 bytes");
static_assert(sizeof(vm_cell) == 8, "x64 jit: a VM number is 8 bytes");

namespace x64jit {

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

uint64_t bits_of(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

X64Generator::X64Generator(x86::Assembler &assembler, sharp_function *fn, jit_compiled_function *out)
    : count(fn->bytecodeSize), a(assembler), function(fn), record(out), code(fn->bytecode) {}

// ---------------------------------------------------------------------------------
// The whole function
// ---------------------------------------------------------------------------------

/**
 * The layout of a compiled function:
 *
 *      entry                       sections/entry.cpp
 *      instruction 0               opcode/...
 *      instruction 1
 *      ...
 *      slow paths                  sections/slow_paths.cpp
 *      resume check + dispatch     sections/dispatch.cpp
 *      exception                   sections/exception.cpp
 *      exits + epilogue            sections/exits.cpp
 */
bool X64Generator::generate() {
    // where the instructions start (a two word instruction's second word is not one)
    starts.assign(count, false);
    for(Int at = 0; at < count; at += instruction_length(GET_OP(code[at])))
        starts[at] = true;

    labels.resize(count);
    for(Int at = 0; at < count; at++)
        labels[at] = a.newLabel();

    directEntry = a.newLabel();
    resumeEntry = a.newLabel();
    resumeCheck = a.newLabel();
    dispatch = a.newLabel();
    exception = a.newLabel();
    exitSwitch = a.newLabel();
    exitKill = a.newLabel();
    leave = a.newLabel();
    epilogue = a.newLabel();
    decline = a.newLabel();
    resumeFromRegisters = a.newLabel();
    declineFromRegisters = a.newLabel();
    callNotFinished = a.newLabel();

    emit_entry();

    for(pc = 0; pc < count; pc = next) {
        decode();
        a.bind(labels[pc]);
        if(!emit_instruction())
            return false;
    }

    // running off the end of the bytecode cannot happen in a well formed
    // function (it ends in RET); if it does, hand back to the interpreter
    a.jmp(declineFromRegisters);

    emit_slow_paths();
    emit_dispatch();
    emit_exception();
    emit_exits();
    return true;
}

/** take the instruction at `pc` apart (the interpreter's single_arg, dual_arg1 ... macros) */
void X64Generator::decode() {
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
bool X64Generator::emit_instruction() {
    emit_result (X64Generator::*kinds[])() = {
        &X64Generator::emit_integer_math,
        &X64Generator::emit_decimal_math,
        &X64Generator::emit_stack_and_locals,
        &X64Generator::emit_objects,
        &X64Generator::emit_fields_and_arrays,
        &X64Generator::emit_allocation,
        &X64Generator::emit_casts_and_locks,
        &X64Generator::emit_control_flow,
        &X64Generator::emit_calls,
        &X64Generator::emit_console
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
// VM registers: homes, and the two modes
// ---------------------------------------------------------------------------------

bool X64Generator::has_cpu_register(Int vmRegister) {
    for(int i = 0; i < HOME_COUNT; i++)
        if(HOME_VM_REGISTER[i] == vmRegister) return true;
    return false;
}

x86::Gp X64Generator::cpu_register_of(Int vmRegister) {
    for(int i = 0; i < HOME_COUNT; i++)
        if(HOME_VM_REGISTER[i] == vmRegister) return HOME_CPU_REGISTER[i];
    return x86::Gp();   // not reached when has_cpu_register() was asked first
}

x86::Mem X64Generator::vm_register_in_memory(Int vmRegister) {
    return x86::qword_ptr(REGS, (int32_t)(vmRegister * 8));
}

/** the operand of an instruction that takes "register or memory" (a.emit(...)) */
Operand X64Generator::vm_register(Int vmRegister) {
    if(has_cpu_register(vmRegister)) return cpu_register_of(vmRegister);
    return vm_register_in_memory(vmRegister);
}

/**
 * The value of a VM register in a CPU register, to read it: its home when it
 * has one (nothing is emitted), otherwise it is loaded into `scratch`.
 * Do not write to the result - use store_vm_register().
 */
x86::Gp X64Generator::vm_register_value(Int vmRegister, const x86::Gp &scratch) {
    if(has_cpu_register(vmRegister)) return cpu_register_of(vmRegister);
    a.mov(scratch, vm_register_in_memory(vmRegister));
    return scratch;
}

void X64Generator::load_vm_register(const x86::Gp &to, Int vmRegister) {
    if(has_cpu_register(vmRegister)) a.mov(to, cpu_register_of(vmRegister));
    else a.mov(to, vm_register_in_memory(vmRegister));
}

void X64Generator::store_vm_register(Int vmRegister, const x86::Gp &from) {
    if(has_cpu_register(vmRegister)) {
        if(cpu_register_of(vmRegister).id() != from.id()) a.mov(cpu_register_of(vmRegister), from);
    }
    else a.mov(vm_register_in_memory(vmRegister), from);
}

void X64Generator::store_vm_register_constant(Int vmRegister, uint64_t bits) {
    if(has_cpu_register(vmRegister)) {
        if(bits == 0) a.xor_(cpu_register_of(vmRegister).r32(), cpu_register_of(vmRegister).r32());
        else a.mov(cpu_register_of(vmRegister), bits);
    } else if((int64_t)bits == (int64_t)(int32_t)bits) {
        a.mov(vm_register_in_memory(vmRegister), (int32_t)bits);      // sign-extended to 64 bits
    } else {
        a.mov(rax, bits);
        a.mov(vm_register_in_memory(vmRegister), rax);
    }
}

/** the decimal in a VM register, in `scratch` (the bits move, nothing is converted) */
x86::Xmm X64Generator::vm_register_decimal(Int vmRegister, const x86::Xmm &scratch) {
    if(has_cpu_register(vmRegister)) a.movq(scratch, cpu_register_of(vmRegister));
    else a.movsd(scratch, vm_register_in_memory(vmRegister));
    return scratch;
}

void X64Generator::store_vm_register_decimal(Int vmRegister, const x86::Xmm &from) {
    if(has_cpu_register(vmRegister)) a.movq(cpu_register_of(vmRegister), from);
    else a.movsd(vm_register_in_memory(vmRegister), from);
}

void X64Generator::save_vm_registers() {
    for(int i = 0; i < HOME_COUNT; i++)
        a.mov(vm_register_in_memory(HOME_VM_REGISTER[i]), HOME_CPU_REGISTER[i]);
}

void X64Generator::load_vm_registers() {
    for(int i = 0; i < HOME_COUNT; i++)
        a.mov(HOME_CPU_REGISTER[i], vm_register_in_memory(HOME_VM_REGISTER[i]));
}

// ---------------------------------------------------------------------------------
// Other operands
// ---------------------------------------------------------------------------------

x86::Mem X64Generator::local_number(Int n)         { return x86::qword_ptr(FP, (int32_t)(n * 16)); }
x86::Mem X64Generator::local_object(Int n)   { return x86::qword_ptr(FP, (int32_t)(n * 16 + 8)); }
x86::Mem X64Generator::stack_number(Int n)           { return x86::qword_ptr(SP, (int32_t)(n * 16)); }
x86::Mem X64Generator::stack_object(Int n)     { return x86::qword_ptr(SP, (int32_t)(n * 16 + 8)); }
x86::Mem X64Generator::fiber_field(size_t offset)  { return x86::qword_ptr(TASK, (int32_t)offset); }
x86::Mem X64Generator::thread_signal()             { return x86::qword_ptr(THREAD, offsetof(sharp_thread, signal)); }
x86::Mem X64Generator::machine_frame_slot(int slot)      { return x86::qword_ptr(x86::rsp, slot); }

// ---------------------------------------------------------------------------------
// The fiber's copies of fp / sp / ptr
// ---------------------------------------------------------------------------------

void X64Generator::save_stack_registers() {
    a.mov(fiber_field(offsetof(fiber, sp)), SP);
    a.mov(fiber_field(offsetof(fiber, ptr)), PTR);
}

void X64Generator::load_stack_registers() {
    a.mov(FP, fiber_field(offsetof(fiber, fp)));
    a.mov(SP, fiber_field(offsetof(fiber, sp)));
    a.mov(PTR, fiber_field(offsetof(fiber, ptr)));
}

void X64Generator::set_current_instruction(Int at) {
    a.mov(rax, (uint64_t)(code + at));
    a.mov(fiber_field(offsetof(fiber, pc)), rax);
}

// ---------------------------------------------------------------------------------
// Calling C++
//
//      prepare_runtime_call();                   // -> memory mode, fiber current
//      a.mov(rdi, vm_register_in_memory(n)); ...     // arguments: from MEMORY, the homes
//                                          // include rdi and rsi themselves
//      call_runtime_checked(jit_rt_...);           // (or call_runtime())
//      load_vm_registers();                          // -> register mode
//      store_vm_register(n, rax);                      // the result
// ---------------------------------------------------------------------------------

//
// TWO WAYS A C++ FUNCTION TAKES ITS ARGUMENTS on x86-64, by system:
//
//      Linux, macOS (System V)     rdi, rsi, rdx, rcx
//      Windows                     rcx, rdx, r8,  r9  - and the caller leaves 32 bytes
//                                  of stack free above the return address for the callee
//
// Everything in opcode/ and sections/ places arguments the first way. On
// Windows this is the ONE place that knows the difference: the four registers
// are moved over (in an order that reads each before it is written), the 32
// bytes are made, and they are given back after the call. r8 and r9 are homes
// of VM registers, and free here: a call into C++ is only made in memory mode.
//
// (Decimals need nothing: a helper takes either whole numbers or decimals, and
// the first two decimals are in xmm0 and xmm1 on both systems.)
//
void X64Generator::call_runtime(void *target) {
#ifdef _WIN32
    a.mov(x86::r9, rcx);
    a.mov(x86::r8, rdx);
    a.mov(rdx, rsi);
    a.mov(rcx, rdi);
    a.mov(rax, (uint64_t)target);
    a.sub(x86::rsp, 32);
    a.call(rax);
    a.add(x86::rsp, 32);
#else
    a.mov(rax, (uint64_t)target);
    a.call(rax);
#endif
}

void X64Generator::call_runtime_checked(void *target) {
    call_runtime(target);
    a.test(thread_signal(), 1 << tsig_except);
    a.jnz(exception);                       // memory mode, as the exception section expects
}

void X64Generator::prepare_runtime_call() {       // jit_runtime.h, rule 2
    set_current_instruction(pc);
    save_stack_registers();
    save_vm_registers();
}

// ---------------------------------------------------------------------------------
// The stack
// ---------------------------------------------------------------------------------

Label X64Generator::request_slow_path(slow_kind kind, Int at, Label back) {
    slow_path_request path;
    path.kind = kind;
    path.entry = a.newLabel();
    path.back = back;
    path.pc = at;
    slowPaths.push_back(path);
    return path.entry;
}

/**
 * (++sp): room for one more slot, like grow_stack + stack_overflow_check.
 * task->growAt is the last slot that can be filled without growing.
 */
void X64Generator::push_stack_slot() {
    Label back = a.newLabel();
    a.cmp(SP, fiber_field(offsetof(fiber, growAt)));
    a.jae(request_slow_path(slow_grow, pc, back));
    a.bind(back);
    a.add(SP, 16);
}

void X64Generator::pop_stack_slot() {
    a.sub(SP, 16);
}

// ---------------------------------------------------------------------------------
// The object pointer
// ---------------------------------------------------------------------------------

/** rax = ptr->o, or a nullptr_exception (the interpreter's require_object_with_value) */
void X64Generator::load_object_or_throw() {
    Label null = request_slow_path(slow_null, pc);
    a.test(PTR, PTR);
    a.jz(null);
    a.mov(rax, x86::qword_ptr(PTR));
    a.test(rax, rax);
    a.jz(null);
}

/** rax = ptr ? ptr->o : null   (what copy_object(to, ptr) stores) */
void X64Generator::load_object_or_null() {
    Label done = a.newLabel();
    a.xor_(x86::eax, x86::eax);
    a.test(PTR, PTR);
    a.jz(done);
    a.mov(rax, x86::qword_ptr(PTR));
    a.bind(done);
}

// ---------------------------------------------------------------------------------
// Integers
// ---------------------------------------------------------------------------------

/**
 * result = left op right, for add / sub / imul / and / or / xor.
 *
 * With all three in homes and result == left this is ONE instruction:
 *      add ecx, ebx -> ecx         add r9, rsi
 * otherwise it goes through rax:
 *      add ecx, ebx -> bmr         mov rax, r9 / add rax, rsi / mov r11, rax
 */
void X64Generator::integer_operation(InstId inst, Int left, Int right, Int result) {
    if(has_cpu_register(result) && result == left) {
        a.emit(inst, cpu_register_of(result), vm_register(right));
        return;
    }

    load_vm_register(rax, left);
    a.emit(inst, rax, vm_register(right));
    store_vm_register(result, rax);
}

/** out = (left ? right) as 0 or 1; `set` picks the comparison */
void X64Generator::integer_compare(Int left, Int right, Int out, void (X64Generator::*set)()) {
    a.emit(x86::Inst::kIdCmp, vm_register_value(left, rax), vm_register(right));
    store_condition(out, set);
}

void X64Generator::store_condition(Int out, void (X64Generator::*set)()) {
    (this->*set)();
    a.movzx(x86::eax, x86::al);
    store_vm_register(out, rax);
}

void X64Generator::condition_less()  { a.setl(x86::al); }
void X64Generator::condition_greater()  { a.setg(x86::al); }
void X64Generator::condition_less_or_equal() { a.setle(x86::al); }
void X64Generator::condition_greater_or_equal() { a.setge(x86::al); }
void X64Generator::condition_equal()  { a.sete(x86::al); }
void X64Generator::condition_not_equal() { a.setne(x86::al); }
void X64Generator::condition_above()  { a.seta(x86::al); }
void X64Generator::condition_above_or_equal() { a.setae(x86::al); }
// ucomisd marks "one of them is a NaN" with the parity flag
void X64Generator::condition_decimal_equal()  { a.setnp(x86::cl); a.sete(x86::al);  a.and_(x86::al, x86::cl); }
void X64Generator::condition_decimal_not_equal() { a.setp(x86::cl);  a.setne(x86::al); a.or_(x86::al, x86::cl); }

// ---------------------------------------------------------------------------------
// Decimals
// ---------------------------------------------------------------------------------

void X64Generator::load_decimal_constant(double value) {
    a.mov(rax, bits_of(value));
    a.movq(xmm1, rax);
}

/** result = left op right, for addsd / subsd / mulsd / divsd */
void X64Generator::decimal_operation(InstId inst, Int left, Int right, Int result) {
    vm_register_decimal(left, xmm0);
    if(has_cpu_register(right)) a.emit(inst, xmm0, vm_register_decimal(right, xmm1));
    else a.emit(inst, xmm0, vm_register_in_memory(right));
    store_vm_register_decimal(result, xmm0);
}

/** regs[n] = regs[n] op value */
void X64Generator::decimal_operation_with_constant(InstId inst, Int vmRegister, double value) {
    load_decimal_constant(value);
    vm_register_decimal(vmRegister, xmm0);
    a.emit(inst, xmm0, xmm1);
    store_vm_register_decimal(vmRegister, xmm0);
}

/** the flags of comparing two decimals (ucomisd left, right) */
void X64Generator::decimal_compare(Int left, Int right) {
    vm_register_decimal(left, xmm0);
    if(has_cpu_register(right)) a.ucomisd(xmm0, vm_register_decimal(right, xmm1));
    else a.ucomisd(xmm0, vm_register_in_memory(right));
}

// ---------------------------------------------------------------------------------
// Jumps and calls
// ---------------------------------------------------------------------------------

/** jump to the instruction at `target`; false when there is none */
bool X64Generator::jump_to_instruction(Int target) {
    if(target < 0 || target >= count || !starts[target]) return false;

    // a jump backwards is the bottom of a loop: the place a running thread
    // looks at its signals (the interpreter's check_state after a jump)
    if(target <= pc) {
        a.cmp(thread_signal(), 0);
        a.jne(request_slow_path(slow_safepoint, target));
    }
    a.jmp(labels[target]);
    return true;
}

/** goto the instruction whose pc is in rax (BRH / IFE / IFNE) */
void X64Generator::jump_to_computed_instruction() {
    save_vm_registers();                                // dispatch is a memory mode section
    a.jmp(dispatch);
}

/**
 * After a helper that ran other Sharp code (jit_rt_call ...). Memory mode; the
 * callee has already moved task->pc.
 *      if(task->current != this function) return JIT_CONTEXT_SWITCH;   // it did not finish
 *      if(thread->signal) -> the resume check (exception, safe point, then dispatch)
 * continuesHere = false: the code after the call is wherever task->pc points
 * (invoke_delegate), not the next instruction.
 */
void X64Generator::check_after_runtime_call(bool continuesHere) {
    a.mov(rax, fiber_field(offsetof(fiber, current)));
    a.mov(rcx, (uint64_t)function);
    a.cmp(rax, rcx);
    a.jne(exitSwitch);
    load_stack_registers();
    if(continuesHere) {
        a.cmp(thread_signal(), 0);
        a.jne(resumeCheck);
        load_vm_registers();
    } else {
        a.jmp(resumeCheck);
    }
}

} // namespace x64jit
