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
    doorEntry = a.newLabel();
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

    pick_slot_homes();
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
        &X64Generator::emit_console,
        &X64Generator::emit_slot_forms
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
    load_vm_registers_only();
    load_slot_homes();          // C++ ran: it is free to use their registers
}

void X64Generator::load_vm_registers_only() {
    for(int i = 0; i < HOME_COUNT; i++)
        a.mov(HOME_CPU_REGISTER[i], vm_register_in_memory(HOME_VM_REGISTER[i]));
}

// ---------------------------------------------------------------------------------
// LOCAL NUMBERS IN CPU REGISTERS: the slot homes
//
// The numbers of a function live in its frame slots, (fp+n)->var. The slots it uses most - found
// by pick_slot_homes(), inner loops first - get a CPU register each for as long as the function's
// machine code runs:
//
//      r8 r9 r10 r11           four slots used as whole numbers
//      xmm2 xmm3 xmm4 xmm5     four slots used as decimals
//
// A home is a COPY that saves the reads. Every write goes to memory AND to the home (store_slot,
// store_slot_decimal, store_slot_constant; sync_slot_home after a write that went to memory only),
// so the slot in memory is right at every moment: the interpreter can take over at any
// instruction, the collector and C++ see what they expect, and there is no second "mode". What a
// loop gains is that `i`, `total` and the like no longer go through memory to reach the next
// instruction - the adds and compares are register to register.
//
// The homes are read again (load_slot_homes) whenever memory may have changed behind them or the
// registers were given away:
//      - after C++ ran: load_vm_registers(), which every return to register mode goes through
//      - when fp is loaded: load_stack_registers() (the stack moved, another frame ran)
//      - at the direct door, and after a direct call comes back (the callee has homes of its own
//        in the same registers)
//
// A slot is 64 bits that are a whole number or a decimal; a home holds the bits either way, the
// kind only decides which register is the cheaper one. A function with an instruction that writes
// a slot's number through sp or the fiber's stack (smovr, isadd, disadd, mov_abs: asm blocks) has
// no homes at all: such a write could be to one of its own slots.
//
// SHARP_X64_SLOT_HOMES=0 in the environment: no homes (to compare).
// ---------------------------------------------------------------------------------

static const x86::Gp WHOLE_HOME[SLOT_HOME_COUNT] = { x86::r8, x86::r9, x86::r10, x86::r11 };
static const x86::Xmm DECIMAL_HOME[10] = { x86::xmm2, x86::xmm3, x86::xmm4, x86::xmm5, x86::xmm6, x86::xmm7,
                                             x86::xmm8, x86::xmm9, x86::xmm10, x86::xmm11 };

int X64Generator::whole_home(Int slot) {
    for(int i = 0; i < wholeHomeCount; i++) if(wholeHomes[i] == slot) return i;
    return -1;
}

int X64Generator::decimal_home(Int slot) {
    for(int i = 0; i < decimalHomeCount; i++) if(decimalHomes[i] == slot) return i;
    return -1;
}

x86::Gp X64Generator::whole_home_register(int home) { return WHOLE_HOME[home]; }
x86::Xmm X64Generator::decimal_home_register(int home) { return DECIMAL_HOME[home]; }

void X64Generator::load_slot_homes() {
    for(int i = 0; i < wholeHomeCount; i++) a.mov(WHOLE_HOME[i], local_number(wholeHomes[i]));
    for(int i = 0; i < decimalHomeCount; i++) a.movsd(DECIMAL_HOME[i], local_number(decimalHomes[i]));
}

void X64Generator::sync_slot_home(Int slot) {
    int home = whole_home(slot);
    if(home >= 0) a.mov(WHOLE_HOME[home], local_number(slot));
    home = decimal_home(slot);
    if(home >= 0) a.movsd(DECIMAL_HOME[home], local_number(slot));
}

x86::Gp X64Generator::slot_in_register(Int slot, const x86::Gp &scratch) {
    int home = whole_home(slot);
    if(home >= 0) return WHOLE_HOME[home];
    // a slot with a decimal home: its bits from there, not from memory - the value was written a
    // moment ago as a rule, and a load that has to wait for that store holds up everything after it
    home = decimal_home(slot);
    if(home >= 0) a.movq(scratch, DECIMAL_HOME[home]);
    else a.mov(scratch, local_number(slot));
    return scratch;
}

Operand X64Generator::slot_operand(Int slot) {
    int home = whole_home(slot);
    if(home >= 0) return WHOLE_HOME[home];
    return local_number(slot);
}

x86::Xmm X64Generator::slot_in_decimal_register(Int slot, const x86::Xmm &scratch) {
    int home = decimal_home(slot);
    if(home >= 0) return DECIMAL_HOME[home];
    home = whole_home(slot);                    // (the same the other way round)
    if(home >= 0) a.movq(scratch, WHOLE_HOME[home]);
    else a.movsd(scratch, local_number(slot));
    return scratch;
}

Operand X64Generator::slot_decimal_operand(Int slot) {
    int home = decimal_home(slot);
    if(home >= 0) return DECIMAL_HOME[home];
    return local_number(slot);
}

/**
 * The register to compute the result for `slot` in: the slot's own home, so that it is not
 * computed in rax and moved - or rax when the slot has none. `unlessSlot`: an operand that is
 * still to be read AFTER the result register is first written (the right side of `d = a op b`);
 * when that is the same slot, its home cannot be written yet, and rax it is.
 */
x86::Gp X64Generator::whole_result_register(Int slot, Int unlessSlot) {
    int home = whole_home(slot);
    return home >= 0 && slot != unlessSlot ? WHOLE_HOME[home] : rax;
}

x86::Xmm X64Generator::decimal_result_register(Int slot, Int unlessSlot) {
    int home = decimal_home(slot);
    return home >= 0 && slot != unlessSlot ? DECIMAL_HOME[home] : xmm0;
}

void X64Generator::store_slot(Int slot, const x86::Gp &from) {
    a.mov(local_number(slot), from);
    int home = whole_home(slot);
    if(home >= 0 && WHOLE_HOME[home].id() != from.id()) a.mov(WHOLE_HOME[home], from);
    home = decimal_home(slot);
    if(home >= 0) a.movq(DECIMAL_HOME[home], from);
}

void X64Generator::store_slot_decimal(Int slot, const x86::Xmm &from) {
    a.movsd(local_number(slot), from);
    int home = decimal_home(slot);
    if(home >= 0 && DECIMAL_HOME[home].id() != from.id()) a.movaps(DECIMAL_HOME[home], from);
    home = whole_home(slot);
    if(home >= 0) a.movq(WHOLE_HOME[home], from);
}

void X64Generator::store_slot_constant(Int slot, uint64_t bits) {
    int whole = whole_home(slot), decimal = decimal_home(slot);
    if((int64_t)bits == (int64_t)(int32_t)bits && decimal < 0) {
        a.mov(local_number(slot), (int32_t)bits);       // sign-extended to 64 bits
        if(whole >= 0) {
            if(bits == 0) a.xor_(WHOLE_HOME[whole].r32(), WHOLE_HOME[whole].r32());
            else a.mov(WHOLE_HOME[whole], bits);
        }
        return;
    }
    a.mov(rax, bits);
    a.mov(local_number(slot), rax);
    if(whole >= 0) a.mov(WHOLE_HOME[whole], rax);
    if(decimal >= 0) a.movq(DECIMAL_HOME[decimal], rax);
}

/**
 * Which slots get a home: every use of a slot as a number is counted, a use inside a loop eight
 * times, inside two loops sixty-four times (a loop is what a backward jump encloses); the four
 * heaviest that are mostly whole numbers and the four heaviest that are mostly decimals win.
 */
void X64Generator::pick_slot_homes() {
    wholeHomeCount = decimalHomeCount = 0;
    static const bool wanted = [] {
        const char *text = getenv("SHARP_X64_SLOT_HOMES");
        return text == nullptr || text[0] != '0';
    }();
    if(!wanted) return;

    // how deep in loops each instruction is
    std::vector<uint8_t> depth((size_t)count, 0);
    for(Int at = 0; at < count; at += instruction_length(GET_OP(code[at]))) {
        const uint32_t word = code[at];
        const int o = GET_OP(word);
        Int target = -1;
        if(o == Opcode::JMP || o == Opcode::JE || o == Opcode::JNE) target = GET_Da(word);
        else if(((o >= Opcode::JLT_S && o <= Opcode::DJNE_S) || o == Opcode::JNZ_S || o == Opcode::JZ_S || o == Opcode::JNULL_S
                 || (o >= Opcode::JLT_K && o <= Opcode::DJNE_K))
                && at + 1 < count) target = (int32_t)code[at + 1];
        if(target >= 0 && target <= at)
            for(Int q = target; q <= at; q++) if(depth[(size_t)q] < 3) depth[(size_t)q]++;
    }

    struct weight { double whole = 0, decimal = 0, either = 0; };
    std::vector<weight> weights;
    double now = 1;
    double reloads = 0;         // how often the homes are read again: once per call into the function, and after every call out of it
    auto use = [&](Int slot, int kind) {         // kind 0: either, 1: whole, 2: decimal
        if(slot < 0 || slot > 1000) return;
        if((size_t)slot >= weights.size()) weights.resize((size_t)slot + 1);
        if(kind == 1) weights[(size_t)slot].whole += now;
        else if(kind == 2) weights[(size_t)slot].decimal += now;
        else weights[(size_t)slot].either += now;
    };
    for(Int at = 0; at < count; at += instruction_length(GET_OP(code[at]))) {
        const uint32_t word = code[at];
        const int o = GET_OP(word);
        now = (double)(1 << (3 * depth[(size_t)at]));
        switch(o) {
            // (after each of these the homes are loaded again: C++ ran, or another function)
            case Opcode::CALL: case Opcode::CALLD: case Opcode::INVOKE_DELEGATE: case Opcode::INT:
            case Opcode::NEWCLASS: case Opcode::NEWARRAY: case Opcode::NEWOBJARRAY: case Opcode::NEWCLASSARRAY:
            case Opcode::NEWSTRING: case Opcode::CAST: case Opcode::VARCAST: case Opcode::IS:
            case Opcode::MOD: case Opcode::IMOD:
                reloads += now;
                break;
            // a number written through sp or the fiber's stack could be one of our slots: no homes
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
            case Opcode::SMOVR_4: use(GET_Da(word), 0); if(at + 1 < count) use((int32_t)code[at + 1], 0); break;
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

    for(int round = 0; round < SLOT_HOME_COUNT + DECIMAL_HOME_COUNT; round++) {
        int best = -1;
        // a home saves a read per use and costs one per reload: it must be used clearly more often than
        // it is reloaded (and a slot named once gains nothing)
        double bestTotal = 1.5 + 0.5 * reloads;
        for(size_t slot = 0; slot < weights.size(); slot++) {
            const weight &w = weights[slot];
            double total = w.whole + w.decimal + w.either;
            bool asDecimal = w.decimal > w.whole;
            if(asDecimal ? decimalHomeCount >= DECIMAL_HOME_COUNT : wholeHomeCount >= SLOT_HOME_COUNT) continue;
            if(total > bestTotal) { bestTotal = total; best = (int)slot; }
        }
        if(best < 0) break;
        weight &w = weights[(size_t)best];
        if(w.decimal > w.whole) decimalHomes[decimalHomeCount++] = best;
        else wholeHomes[wholeHomeCount++] = best;
        w.whole = w.decimal = w.either = 0;
    }
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
    load_slot_homes();          // a new fp: the frame moved, or other code ran on this stack
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
// of frame slots (x64_generator.cpp, LOCAL NUMBERS IN CPU REGISTERS), and free
// here: memory is always right for those, and they are read again after the call.
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
 * With the result in a home and result == left this is ONE instruction:
 *      add ebx, ecx -> ebx         add rsi, [r14 + ecx*8]
 * otherwise it goes through rax:
 *      add ecx, ebx -> bmr         mov rax, [r14 + ecx*8] / add rax, rsi / mov [r14 + bmr*8], rax
 * (These are the register forms. What the compiler writes for `d = a + b` today is the slot
 *  form add_s: opcode/slot_forms.cpp.)
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

/**
 * A jump that compares and jumps in one (the slot forms) calls this first, then compares, then
 * jumps straight to labels[target]. For a jump BACKWARDS - the bottom of a loop - it is the safe
 * point: the thread looks at its signals here, before the comparison (afterwards it would spoil
 * the comparison's flags), and comes back to THIS instruction when it has dealt with them. So
 * the loop's own path is: one test of the signal word, the comparison, one jump taken.
 * false: `target` is not an instruction.
 */
bool X64Generator::before_conditional_jump(Int target) {
    if(target < 0 || target >= count || !starts[target]) return false;
    if(target <= pc) {
        a.cmp(thread_signal(), 0);
        a.jne(request_slow_path(slow_safepoint, pc));
    }
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
