//
// THE ARM JIT ON 64-BIT ARM (AArch64): the operations of arm_machine.h as machine code, written
// with the AsmJit library (the same one the x86-64 JIT uses).
//
// THE REGISTERS
//      x19 thread   x20 task   x21 regs   x22 fp   x23 sp   x24 ptr      reserved: C++ gives them back
//      x11 x12 x13                                                       pointer scratch (PA PB PC)
//      x9 x10                                                            value scratch (V0 V1)
//      d16 d17                                                           decimal scratch (D0 D1)
//      x16 x17                                                           this file's own: a large offset,
//                                                                        a constant, the address of a call
//      x0 .. x7, d0, d1                                                  arguments and results of C++
//
// A value register IS a pointer register here (both are 64 bits), so "widen", "narrow" and "store
// an object" are plain moves and stores.
//
// A CALL INTO C++ needs nothing saved: the scratch registers are scratch for C++ too, and the six
// reserved ones are among those every C++ function hands back.
//
// NUMBERS THAT LIVE IN CPU REGISTERS ("homes")
//      x14 x15 x1 x2 x3 x4 x5 x6 x7 x8     the ten VM registers (adx, cx, cmt, ebx ... egx)
//      x25 x26 x27 x28                     the four locals the function uses most as whole numbers
//      d8 d9 d10 d11                       the four it uses most as decimals
//
// A home is a COPY that saves the reads. Every write goes to the home AND to memory, so memory is
// right at every moment: the interpreter can take over at any instruction, C++ sees what it expects,
// and no "which of the two is current" has to be tracked. What has to be kept is the other way
// round - the home must be read again whenever memory may have changed behind it:
//      the VM registers    when the body is entered, and after every call into C++ (it can run
//                          other Sharp code, which uses the same register array - and the homes of
//                          the VM registers are registers C++ is free to use anyway)
//      the locals          whenever FP is loaded (the door; after anything that can move or change
//                          the stack), and when the generator says so (stack_numbers_changed).
//                          Their homes are registers C++ hands back, so a call that leaves the
//                          stack alone costs nothing.
// A local is 64 bits that are a whole number or a decimal; its home is of the kind it is mostly
// used as, and the other kind of use reads memory (which is right) and writes both.
//
// WORKING IN THE HOMES. `add_s d, a, b` with all three at home is one `add` and the store that
// keeps memory right: the slot_...() operations read each local where it is (whole_local,
// decimal_local) and compute into the result's own home (whole_result, decimal_result).
//
// SHARP_ARM_HOMES=0 / 1 / 2 in the environment: none / the VM registers / both (the default).
//
#include "arm64_machine.h"

#ifdef SHARP_JIT_ARM64

#include <cstring>

using namespace asmjit;

namespace armjit {

static const a64::Gp TEMP = a64::x17;       // a large offset, a constant
static const a64::Gp CALL = a64::x16;       // the address of a call or a jump

static int homes_wanted() {
    static const int wanted = [] {
        const char *text = getenv("SHARP_ARM_HOMES");
        return text != nullptr && text[0] >= '0' && text[0] <= '2' ? text[0] - '0' : 2;
    }();
    return wanted;
}

Arm64Machine::Arm64Machine(a64::Assembler &assembler, JitErrorHandler &handler)
    : a(assembler), errors(handler), homes(homes_wanted()) {}

static const a64::Gp VM_HOME[10] = { a64::x14, a64::x15, a64::x1, a64::x2, a64::x3, a64::x4, a64::x5, a64::x6, a64::x7, a64::x8 };
static const a64::Gp WHOLE_HOME[4] = { a64::x25, a64::x26, a64::x27, a64::x28 };
static const a64::Vec DECIMAL_HOME[4] = { a64::d8, a64::d9, a64::d10, a64::d11 };

int Arm64Machine::vm_home(PReg base, int32_t offset) {
    if(homes < 1 || base != REGS || offset < 0 || offset >= 80 || offset % 8 != 0) return -1;
    return offset / 8;
}

int Arm64Machine::whole_home(PReg base, int32_t offset) {
    if(base != FP) return -1;
    for(size_t i = 0; i < whole_homes.size(); i++)
        if(whole_homes[i] == offset) return (int) i;
    return -1;
}

int Arm64Machine::decimal_home(PReg base, int32_t offset) {
    if(base != FP) return -1;
    for(size_t i = 0; i < decimal_homes.size(); i++)
        if(decimal_homes[i] == offset) return (int) i;
    return -1;
}

void Arm64Machine::reload_vm_homes() {
    if(homes < 1) return;
    for(int i = 0; i < 10; i += 2)
        a.ldp(VM_HOME[i], VM_HOME[i + 1], a64::ptr(a64::x21, i * 8));
}

void Arm64Machine::reload_local_homes() {
    for(size_t i = 0; i < whole_homes.size(); i++)
        a.ldr(WHOLE_HOME[i], at(FP, whole_homes[i], 8));
    for(size_t i = 0; i < decimal_homes.size(); i++)
        a.ldr(DECIMAL_HOME[i], at(FP, decimal_homes[i], 8));
}

void Arm64Machine::home_locals(const std::vector<int32_t> &whole, const std::vector<int32_t> &decimal) {
    if(homes < 2) return;
    whole_homes.assign(whole.begin(), whole.begin() + (whole.size() < 4 ? whole.size() : 4));
    decimal_homes.assign(decimal.begin(), decimal.begin() + (decimal.size() < 4 ? decimal.size() : 4));
}

void Arm64Machine::stack_numbers_changed() { reload_local_homes(); }

a64::Gp Arm64Machine::p(PReg r) {
    switch(r) {
        case THREAD: return a64::x19;
        case TASK: return a64::x20;
        case REGS: return a64::x21;
        case FP: return a64::x22;
        case SP: return a64::x23;
        case PTR: return a64::x24;
        case PA: return a64::x11;
        case PB: return a64::x12;
        default: return a64::x13;
    }
}

a64::Gp Arm64Machine::v(VReg r) {
    return r == V0 ? a64::x9 : a64::x10;
}

a64::Vec Arm64Machine::d(DReg r) {
    return r == D0 ? a64::d16 : a64::d17;
}

/** any 64 bits into a register: 16 at a time */
void Arm64Machine::constant(const a64::Gp &to, uint64_t value) {
    a.movz(to, Imm(value & 0xFFFF), Imm(0));
    for(int shift = 16; shift < 64; shift += 16) {
        uint64_t part = (value >> shift) & 0xFFFF;
        if(part != 0) a.movk(to, Imm(part), Imm(shift));
    }
}

/**
 * base + offset as an operand of a load or store of `size` bytes. The instruction itself takes an
 * offset of 0 to 4095 times the size, or -256 to 255; anything else is added up in TEMP first.
 */
a64::Mem Arm64Machine::at(PReg base, int32_t offset, int size) {
    bool scaled = offset >= 0 && offset % size == 0 && offset / size < 4096;
    bool small = offset >= -256 && offset <= 255;
    if(scaled || small) return a64::ptr(p(base), offset);
    constant(TEMP, (uint64_t) (int64_t) offset);
    a.add(TEMP, p(base), TEMP);
    return a64::ptr(TEMP);
}

// ---- places in the code

int Arm64Machine::label() {
    labels.push_back(a.newLabel());
    return (int) labels.size() - 1;
}

void Arm64Machine::bind(int which) { a.bind(labels[(size_t) which]); }
void Arm64Machine::jump(int which) { a.b(labels[(size_t) which]); }

// ---- 64-bit whole numbers

void Arm64Machine::v_constant(VReg to, int64_t value) { constant(v(to), (uint64_t) value); }
void Arm64Machine::v_load(VReg to, PReg base, int32_t offset) {
    int home = vm_home(base, offset);
    if(home >= 0) { a.mov(v(to), VM_HOME[home]); return; }
    home = whole_home(base, offset);
    if(home >= 0) { a.mov(v(to), WHOLE_HOME[home]); return; }
    a.ldr(v(to), at(base, offset, 8));
}

void Arm64Machine::v_store(VReg from, PReg base, int32_t offset) {
    a.str(v(from), at(base, offset, 8));
    int home = vm_home(base, offset);
    if(home >= 0) a.mov(VM_HOME[home], v(from));
    home = whole_home(base, offset);
    if(home >= 0) a.mov(WHOLE_HOME[home], v(from));
    home = decimal_home(base, offset);
    if(home >= 0) a.fmov(DECIMAL_HOME[home], v(from));
}

void Arm64Machine::whole_operation(VOp op, const a64::Gp &to, const a64::Gp &left, const a64::Gp &right) {
    switch(op) {
        case V_ADD: a.add(to, left, right); break;
        case V_SUB: a.sub(to, left, right); break;
        case V_MUL: a.mul(to, left, right); break;
        case V_AND: a.and_(to, left, right); break;
        case V_OR: a.orr(to, left, right); break;
        case V_XOR: a.eor(to, left, right); break;
        case V_SHL: a.lsl(to, left, right); break;      // (the count modulo 64, as on a PC)
        case V_SAR: a.asr(to, left, right); break;
    }
}

void Arm64Machine::v_operation(VOp op, VReg to, VReg left, VReg right) { whole_operation(op, v(to), v(left), v(right)); }

void Arm64Machine::v_negate(VReg r) { a.neg(v(r), v(r)); }

void Arm64Machine::v_flip_sign(VReg r) {
    constant(TEMP, 0x8000000000000000ull);
    a.eor(v(r), v(r), TEMP);
}

void Arm64Machine::v_extend(VReg r, int bits, bool is_signed) {
    a64::Gp x = v(r), w = v(r).w();
    if(is_signed) {
        if(bits == 8) a.sxtb(x, w);
        else if(bits == 16) a.sxth(x, w);
        else a.sxtw(x, w);
    } else {
        if(bits == 8) a.uxtb(w, w);
        else if(bits == 16) a.uxth(w, w);
        else a.mov(w, w);                   // (writing the low half clears the high one)
    }
}

static a64::CondCode condition(Cond how) {
    switch(how) {
        case LT: return a64::CondCode::kLT;
        case GT: return a64::CondCode::kGT;
        case LE: return a64::CondCode::kLE;
        case GE: return a64::CondCode::kGE;
        case EQ: return a64::CondCode::kEQ;
        default: return a64::CondCode::kNE;
    }
}

void Arm64Machine::v_compare(VReg to, VReg left, VReg right, Cond how) {
    a.cmp(v(left), v(right));
    a.cset(v(to), Imm((uint32_t) condition(how)));
}

void Arm64Machine::v_jump_if(VReg left, VReg right, Cond how, bool holds, int which) {
    a.cmp(v(left), v(right));
    jump_if_whole(how, holds, which);
}

void Arm64Machine::jump_if_whole(Cond how, bool holds, int which) {
    const Label &to = labels[(size_t) which];
    switch(how) {
        case LT: if(holds) a.b_lt(to); else a.b_ge(to); break;
        case GT: if(holds) a.b_gt(to); else a.b_le(to); break;
        case LE: if(holds) a.b_le(to); else a.b_gt(to); break;
        case GE: if(holds) a.b_ge(to); else a.b_lt(to); break;
        case EQ: if(holds) a.b_eq(to); else a.b_ne(to); break;
        default: if(holds) a.b_ne(to); else a.b_eq(to); break;
    }
}

void Arm64Machine::v_not_zero(VReg r) {
    a.cmp(v(r), Imm(0));
    a.cset(v(r), Imm((uint32_t) a64::CondCode::kNE));
}

void Arm64Machine::v_jump_if_not_below(VReg index, PReg size, int which) {
    a.cmp(v(index), p(size));
    a.b_hs(labels[(size_t) which]);
}

void Arm64Machine::v_jump_if_zero(VReg r, int which) { a.cbz(v(r), labels[(size_t) which]); }
void Arm64Machine::v_jump_if_not_zero(VReg r, int which) { a.cbnz(v(r), labels[(size_t) which]); }
void Arm64Machine::v_from_pointer(VReg to, PReg from) { a.mov(v(to), p(from)); }
void Arm64Machine::v_to_pointer(PReg to, VReg from) { a.mov(p(to), v(from)); }

// ---- decimals

void Arm64Machine::d_constant(DReg to, double value) {
    uint64_t bits;
    memcpy(&bits, &value, 8);
    constant(TEMP, bits);
    a.fmov(d(to), TEMP);
}

void Arm64Machine::d_load(DReg to, PReg base, int32_t offset) {
    int home = vm_home(base, offset);
    if(home >= 0) { a.fmov(d(to), VM_HOME[home]); return; }
    home = decimal_home(base, offset);
    if(home >= 0) { a.fmov(d(to), DECIMAL_HOME[home]); return; }
    a.ldr(d(to), at(base, offset, 8));
}

void Arm64Machine::d_store(DReg from, PReg base, int32_t offset) {
    a.str(d(from), at(base, offset, 8));
    int home = vm_home(base, offset);
    if(home >= 0) a.fmov(VM_HOME[home], d(from));
    home = decimal_home(base, offset);
    if(home >= 0) a.fmov(DECIMAL_HOME[home], d(from));
    home = whole_home(base, offset);
    if(home >= 0) a.fmov(WHOLE_HOME[home], d(from));
}

void Arm64Machine::d_operation(DOp op, DReg to, DReg left, DReg right) {
    switch(op) {
        case D_ADD: a.fadd(d(to), d(left), d(right)); break;
        case D_SUB: a.fsub(d(to), d(left), d(right)); break;
        case D_MUL: a.fmul(d(to), d(left), d(right)); break;
        case D_DIV: a.fdiv(d(to), d(left), d(right)); break;
    }
}

/**
 * A comparison of decimals gives four answers: less, equal, greater, "one is not a number". The
 * conditions are picked so that the last one makes every comparison false - and != true.
 */
void Arm64Machine::d_compare(VReg to, DReg left, DReg right, Cond how) {
    a.fcmp(d(left), d(right));
    a64::CondCode code = how == LT ? a64::CondCode::kMI : how == GT ? a64::CondCode::kGT : how == LE ? a64::CondCode::kLS
                       : how == GE ? a64::CondCode::kGE : how == EQ ? a64::CondCode::kEQ : a64::CondCode::kNE;
    a.cset(v(to), Imm((uint32_t) code));
}

/** (the conditions of d_compare; each "does not hold" is the exact opposite, so it is taken when one is not a number) */
void Arm64Machine::d_jump_if(DReg left, DReg right, Cond how, bool holds, int which) {
    a.fcmp(d(left), d(right));
    jump_if_decimal(how, holds, which);
}

void Arm64Machine::jump_if_decimal(Cond how, bool holds, int which) {
    const Label &to = labels[(size_t) which];
    switch(how) {
        case LT: if(holds) a.b_mi(to); else a.b_pl(to); break;
        case GT: if(holds) a.b_gt(to); else a.b_le(to); break;
        case LE: if(holds) a.b_ls(to); else a.b_hi(to); break;
        case GE: if(holds) a.b_ge(to); else a.b_lt(to); break;
        case EQ: if(holds) a.b_eq(to); else a.b_ne(to); break;
        default: if(holds) a.b_ne(to); else a.b_eq(to); break;
    }
}

void Arm64Machine::d_from_whole(DReg to, VReg from) { a.scvtf(d(to), v(from)); }

// ---- working in the homes

/** the register a local whole number is read from: its home, or `scratch` after loading it there */
a64::Gp Arm64Machine::whole_local(int32_t offset, const a64::Gp &scratch) {
    int home = whole_home(FP, offset);
    if(home >= 0) return WHOLE_HOME[home];
    a.ldr(scratch, at(FP, offset, 8));          // (memory is right also for a local with a decimal home)
    return scratch;
}

/** the register to compute a local's new whole number in: its home, or `scratch` */
a64::Gp Arm64Machine::whole_result(int32_t offset, const a64::Gp &scratch) {
    int home = whole_home(FP, offset);
    return home >= 0 ? WHOLE_HOME[home] : scratch;
}

/** after computing in whole_result(): memory gets the number too (and a decimal home, if that is what the local has) */
void Arm64Machine::whole_local_written(int32_t offset, const a64::Gp &from) {
    a.str(from, at(FP, offset, 8));
    int home = decimal_home(FP, offset);
    if(home >= 0) a.fmov(DECIMAL_HOME[home], from);
}

a64::Vec Arm64Machine::decimal_local(int32_t offset, const a64::Vec &scratch) {
    int home = decimal_home(FP, offset);
    if(home >= 0) return DECIMAL_HOME[home];
    a.ldr(scratch, at(FP, offset, 8));
    return scratch;
}

a64::Vec Arm64Machine::decimal_result(int32_t offset, const a64::Vec &scratch) {
    int home = decimal_home(FP, offset);
    return home >= 0 ? DECIMAL_HOME[home] : scratch;
}

void Arm64Machine::decimal_local_written(int32_t offset, const a64::Vec &from) {
    a.str(from, at(FP, offset, 8));
    int home = whole_home(FP, offset);
    if(home >= 0) a.fmov(WHOLE_HOME[home], from);
}

void Arm64Machine::slot_operation(VOp op, int32_t to, int32_t left, int32_t right) {
    a64::Gp l = whole_local(left, v(V0)), r = whole_local(right, v(V1)), result = whole_result(to, v(V0));
    whole_operation(op, result, l, r);
    whole_local_written(to, result);
}

/** (a small constant is part of the instruction: add, subtract, the shifts) */
void Arm64Machine::slot_operation_constant(VOp op, int32_t to, int32_t left, int64_t right) {
    a64::Gp l = whole_local(left, v(V0)), result = whole_result(to, v(V0));
    bool small = right > -4096 && right < 4096;
    if(op == V_ADD && small) { if(right >= 0) a.add(result, l, Imm(right)); else a.sub(result, l, Imm(-right)); }
    else if(op == V_SUB && small) { if(right >= 0) a.sub(result, l, Imm(right)); else a.add(result, l, Imm(-right)); }
    else if(op == V_SHL) a.lsl(result, l, Imm(right & 63));
    else if(op == V_SAR) a.asr(result, l, Imm(right & 63));
    else {
        constant(v(V1), (uint64_t) right);
        whole_operation(op, result, l, v(V1));
    }
    whole_local_written(to, result);
}

void Arm64Machine::slot_jump_if(int32_t left, int32_t right, Cond how, bool holds, int which) {
    a64::Gp l = whole_local(left, v(V0)), r = whole_local(right, v(V1));
    a.cmp(l, r);
    jump_if_whole(how, holds, which);
}

void Arm64Machine::slot_jump_if_constant(int32_t left, int64_t right, Cond how, bool holds, int which) {
    a64::Gp l = whole_local(left, v(V0));
    if(right >= 0 && right < 4096) a.cmp(l, Imm(right));
    else {
        constant(v(V1), (uint64_t) right);
        a.cmp(l, v(V1));
    }
    jump_if_whole(how, holds, which);
}

void Arm64Machine::slot_jump_if_zero(int32_t slot, bool is_zero, int which) {
    a64::Gp value = whole_local(slot, v(V0));
    if(is_zero) a.cbz(value, labels[(size_t) which]); else a.cbnz(value, labels[(size_t) which]);
}

void Arm64Machine::slot_decimal_operation(DOp op, int32_t to, int32_t left, int32_t right) {
    a64::Vec l = decimal_local(left, d(D0)), r = decimal_local(right, d(D1)), result = decimal_result(to, d(D0));
    switch(op) {
        case D_ADD: a.fadd(result, l, r); break;
        case D_SUB: a.fsub(result, l, r); break;
        case D_MUL: a.fmul(result, l, r); break;
        case D_DIV: a.fdiv(result, l, r); break;
    }
    decimal_local_written(to, result);
}

void Arm64Machine::slot_decimal_jump_if(int32_t left, int32_t right, Cond how, bool holds, int which) {
    a64::Vec l = decimal_local(left, d(D0)), r = decimal_local(right, d(D1));
    a.fcmp(l, r);
    jump_if_decimal(how, holds, which);
}

void Arm64Machine::slot_decimal_jump_if_constant(int32_t left, double right, Cond how, bool holds, int which) {
    a64::Vec l = decimal_local(left, d(D0));
    d_constant(D1, right);
    a.fcmp(l, d(D1));
    jump_if_decimal(how, holds, which);
}

void Arm64Machine::slot_whole_to_decimal(int32_t to, int32_t from) {
    a64::Gp value = whole_local(from, v(V0));
    a64::Vec result = decimal_result(to, d(D0));
    a.scvtf(result, value);
    decimal_local_written(to, result);
}

// ---- pointers

void Arm64Machine::p_constant(PReg to, uintptr_t value) { constant(p(to), (uint64_t) value); }
void Arm64Machine::p_load(PReg to, PReg base, int32_t offset) {
    a.ldr(p(to), at(base, offset, 8));
    if(to == FP) reload_local_homes();          // a new frame pointer: the stack moved, or C++ ran
}
void Arm64Machine::p_store(PReg from, PReg base, int32_t offset) { a.str(p(from), at(base, offset, 8)); }
void Arm64Machine::p_store_object(PReg from, PReg base, int32_t offset) { a.str(p(from), at(base, offset, 8)); }
void Arm64Machine::p_load_32(PReg to, PReg base, int32_t offset) { a.ldr(p(to).w(), at(base, offset, 4)); }
void Arm64Machine::p_load_8(PReg to, PReg base, int32_t offset) { a.ldrb(p(to).w(), at(base, offset, 1)); }
void Arm64Machine::p_load_int(PReg to, PReg base, int32_t offset) { a.ldrsw(p(to), at(base, offset, 4)); }
void Arm64Machine::p_store_32(PReg from, PReg base, int32_t offset) { a.str(p(from).w(), at(base, offset, 4)); }
void Arm64Machine::p_store_8(PReg from, PReg base, int32_t offset) { a.strb(p(from).w(), at(base, offset, 1)); }
void Arm64Machine::p_move(PReg to, PReg from) { a.mov(p(to), p(from)); }

void Arm64Machine::p_multiply(PReg r, uint32_t by) {
    constant(TEMP, by);
    a.mul(p(r), p(r), TEMP);
}

void Arm64Machine::p_and(PReg r, uint32_t mask) {
    constant(TEMP, mask);
    a.and_(p(r), p(r), TEMP);
}

void Arm64Machine::p_jump_if_machine_stack_below(PReg limit, int which) {
    a.mov(TEMP, a64::sp);
    a.cmp(TEMP, p(limit));
    a.b_lo(labels[(size_t) which]);
}

void Arm64Machine::p_add(PReg to, PReg from, int32_t amount) {
    if(amount >= 0 && amount < 4096) a.add(p(to), p(from), Imm(amount));
    else if(amount < 0 && amount > -4096) a.sub(p(to), p(from), Imm(-amount));
    else {
        constant(TEMP, (uint64_t) (int64_t) amount);
        a.add(p(to), p(from), TEMP);
    }
}

void Arm64Machine::p_add_scaled(PReg to, PReg base, PReg index, int shift) {
    a.lsl(TEMP, p(index), Imm(shift));
    a.add(p(to), p(base), TEMP);
}

void Arm64Machine::p_subtract(PReg to, PReg left, PReg right) { a.sub(p(to), p(left), p(right)); }
void Arm64Machine::p_shift_right(PReg r, int bits) { a.asr(p(r), p(r), Imm(bits)); }
void Arm64Machine::p_jump_if_zero(PReg r, int which) { a.cbz(p(r), labels[(size_t) which]); }
void Arm64Machine::p_jump_if_not_zero(PReg r, int which) { a.cbnz(p(r), labels[(size_t) which]); }

void Arm64Machine::p_jump_if(PReg left, PReg right, PCond how, int which) {
    a.cmp(p(left), p(right));
    if(how == P_EQ) a.b_eq(labels[(size_t) which]);
    else if(how == P_NE) a.b_ne(labels[(size_t) which]);
    else a.b_hs(labels[(size_t) which]);
}

void Arm64Machine::p_jump_if_equals(PReg r, int32_t value, int which) {
    constant(TEMP, (uint64_t) (int64_t) value);
    a.cmp(p(r), TEMP);
    a.b_eq(labels[(size_t) which]);
}

void Arm64Machine::p_jump_if_bits(PReg r, uint32_t mask, bool any_set, int which) {
    constant(TEMP, mask);
    a.tst(p(r).w(), TEMP.w());
    if(any_set) a.b_ne(labels[(size_t) which]);
    else a.b_eq(labels[(size_t) which]);
}

// ---- the items of a number array

static int shift_of(int bytes) { return bytes == 1 ? 0 : bytes == 2 ? 1 : bytes == 4 ? 2 : 3; }

void Arm64Machine::item_load(VReg to, PReg base, PReg index, int bytes, bool is_signed) {
    a64::Mem item = bytes == 1 ? a64::ptr(p(base), p(index)) : a64::ptr(p(base), p(index), a64::lsl((uint32_t) shift_of(bytes)));
    a64::Gp x = v(to), w = v(to).w();
    if(bytes == 1) { if(is_signed) a.ldrsb(x, item); else a.ldrb(w, item); }
    else if(bytes == 2) { if(is_signed) a.ldrsh(x, item); else a.ldrh(w, item); }
    else if(bytes == 4) { if(is_signed) a.ldrsw(x, item); else a.ldr(w, item); }
    else a.ldr(x, item);
}

void Arm64Machine::item_store(VReg from, PReg base, PReg index, int bytes) {
    a64::Mem item = bytes == 1 ? a64::ptr(p(base), p(index)) : a64::ptr(p(base), p(index), a64::lsl((uint32_t) shift_of(bytes)));
    a64::Gp x = v(from), w = v(from).w();
    if(bytes == 1) a.strb(w, item);
    else if(bytes == 2) a.strh(w, item);
    else if(bytes == 4) a.str(w, item);
    else a.str(x, item);
}

// ---- C++

/**
 * Whole numbers and pointers go in x0, x1 ... in order, decimals in d0, d1 ... in order - each kind
 * counted on its own. No scratch register is an argument register, so nothing is overwritten
 * before it is read.
 */
void Arm64Machine::call(void *function, const std::vector<Arg> &arguments) {
    static const a64::Gp whole[8] = { a64::x0, a64::x1, a64::x2, a64::x3, a64::x4, a64::x5, a64::x6, a64::x7 };
    static const a64::Vec decimals[8] = { a64::d0, a64::d1, a64::d2, a64::d3, a64::d4, a64::d5, a64::d6, a64::d7 };
    int next_whole = 0, next_decimal = 0;
    for(const Arg &arg : arguments) {
        switch(arg.kind) {
            case Arg::POINTER: a.mov(whole[next_whole++], p(arg.p)); break;
            case Arg::VALUE: a.mov(whole[next_whole++], v(arg.v)); break;
            case Arg::CONSTANT: constant(whole[next_whole++], (uint64_t) arg.c); break;
            case Arg::DECIMAL: a.fmov(decimals[next_decimal++], d(arg.d)); break;
        }
    }
    constant(CALL, (uint64_t) (uintptr_t) function);
    a.blr(CALL);
    reload_vm_homes();          // (x0 and d0, the result, are not among them)
}

void Arm64Machine::result_to_value(VReg to) { a.mov(v(to), a64::x0); }
void Arm64Machine::result_to_pointer(PReg to) { a.mov(p(to), a64::x0); }
void Arm64Machine::result_to_decimal(DReg to) { a.fmov(d(to), a64::d0); }

// ---- the shape of a compiled function

void Arm64Machine::door_enter() {
    // x29 (the frame) and x30 (where to return to), then the six registers we reserve
    a.stp(a64::x29, a64::x30, a64::ptr_pre(a64::sp, -128));
    a.stp(a64::x19, a64::x20, a64::ptr(a64::sp, 16));
    a.stp(a64::x21, a64::x22, a64::ptr(a64::sp, 32));
    a.stp(a64::x23, a64::x24, a64::ptr(a64::sp, 48));
    a.stp(a64::x25, a64::x26, a64::ptr(a64::sp, 64));       // (the homes of the locals)
    a.stp(a64::x27, a64::x28, a64::ptr(a64::sp, 80));
    a.stp(a64::d8, a64::d9, a64::ptr(a64::sp, 96));
    a.stp(a64::d10, a64::d11, a64::ptr(a64::sp, 112));
    a.mov(a64::x19, a64::x0);           // thread
    a.mov(a64::x21, a64::x2);           // registers
}

void Arm64Machine::door_leave() {
    a.ldp(a64::x19, a64::x20, a64::ptr(a64::sp, 16));
    a.ldp(a64::x21, a64::x22, a64::ptr(a64::sp, 32));
    a.ldp(a64::x23, a64::x24, a64::ptr(a64::sp, 48));
    a.ldp(a64::x25, a64::x26, a64::ptr(a64::sp, 64));
    a.ldp(a64::x27, a64::x28, a64::ptr(a64::sp, 80));
    a.ldp(a64::d8, a64::d9, a64::ptr(a64::sp, 96));
    a.ldp(a64::d10, a64::d11, a64::ptr(a64::sp, 112));
    a.ldp(a64::x29, a64::x30, a64::ptr_post(a64::sp, 128));
    a.ret(a64::x30);
}

// The body calls C++, which uses x30 for ITS return address: ours is kept in the body's frame.
// 48 bytes: x29 and x30, the three slots of frame_store(), and 8 to keep sp at a multiple of 16.
void Arm64Machine::body_enter() {
    a.stp(a64::x29, a64::x30, a64::ptr_pre(a64::sp, -48));
    reload_vm_homes();
}

void Arm64Machine::body_return() {
    a.ldp(a64::x29, a64::x30, a64::ptr_post(a64::sp, 48));
    a.ret(a64::x30);
}

void Arm64Machine::call_label(int which) { a.bl(labels[(size_t) which]); }
void Arm64Machine::frame_store(int slot, PReg from) { a.str(p(from), a64::ptr(a64::sp, 16 + slot * 8)); }
void Arm64Machine::frame_load(PReg to, int slot) { a.ldr(p(to), a64::ptr(a64::sp, 16 + slot * 8)); }

void Arm64Machine::call_compiled(PReg target) {
    a.blr(p(target));
    reload_vm_homes();          // the callee wrote VM registers (memory is right: see the top of this file)
}
void Arm64Machine::return_code(int code) { constant(a64::x0, (uint64_t) code); }

void Arm64Machine::jump_through_table(PReg table, PReg index) {
    a.ldr(CALL, a64::ptr(p(table), p(index), a64::lsl(3)));
    a.br(CALL);
}

bool Arm64Machine::failed() { return errors.failed; }

} // namespace armjit

#endif // SHARP_JIT_ARM64
