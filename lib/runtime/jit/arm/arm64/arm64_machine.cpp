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
#include "arm64_machine.h"

#ifdef SHARP_JIT_ARM64

#include <cstring>

using namespace asmjit;

namespace armjit {

static const a64::Gp TEMP = a64::x17;       // a large offset, a constant
static const a64::Gp CALL = a64::x16;       // the address of a call or a jump

Arm64Machine::Arm64Machine(a64::Assembler &assembler, JitErrorHandler &handler) : a(assembler), errors(handler) {}

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
void Arm64Machine::v_load(VReg to, PReg base, int32_t offset) { a.ldr(v(to), at(base, offset, 8)); }
void Arm64Machine::v_store(VReg from, PReg base, int32_t offset) { a.str(v(from), at(base, offset, 8)); }

void Arm64Machine::v_operation(VOp op, VReg to, VReg left, VReg right) {
    switch(op) {
        case V_ADD: a.add(v(to), v(left), v(right)); break;
        case V_SUB: a.sub(v(to), v(left), v(right)); break;
        case V_MUL: a.mul(v(to), v(left), v(right)); break;
        case V_AND: a.and_(v(to), v(left), v(right)); break;
        case V_OR: a.orr(v(to), v(left), v(right)); break;
        case V_XOR: a.eor(v(to), v(left), v(right)); break;
        case V_SHL: a.lsl(v(to), v(left), v(right)); break;     // (the count modulo 64, as on a PC)
        case V_SAR: a.asr(v(to), v(left), v(right)); break;
    }
}

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

void Arm64Machine::d_load(DReg to, PReg base, int32_t offset) { a.ldr(d(to), at(base, offset, 8)); }
void Arm64Machine::d_store(DReg from, PReg base, int32_t offset) { a.str(d(from), at(base, offset, 8)); }

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

void Arm64Machine::d_from_whole(DReg to, VReg from) { a.scvtf(d(to), v(from)); }

// ---- pointers

void Arm64Machine::p_constant(PReg to, uintptr_t value) { constant(p(to), (uint64_t) value); }
void Arm64Machine::p_load(PReg to, PReg base, int32_t offset) { a.ldr(p(to), at(base, offset, 8)); }
void Arm64Machine::p_store(PReg from, PReg base, int32_t offset) { a.str(p(from), at(base, offset, 8)); }
void Arm64Machine::p_store_object(PReg from, PReg base, int32_t offset) { a.str(p(from), at(base, offset, 8)); }
void Arm64Machine::p_load_32(PReg to, PReg base, int32_t offset) { a.ldr(p(to).w(), at(base, offset, 4)); }
void Arm64Machine::p_load_8(PReg to, PReg base, int32_t offset) { a.ldrb(p(to).w(), at(base, offset, 1)); }
void Arm64Machine::p_move(PReg to, PReg from) { a.mov(p(to), p(from)); }

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
}

void Arm64Machine::result_to_value(VReg to) { a.mov(v(to), a64::x0); }
void Arm64Machine::result_to_pointer(PReg to) { a.mov(p(to), a64::x0); }
void Arm64Machine::result_to_decimal(DReg to) { a.fmov(d(to), a64::d0); }

// ---- the shape of a compiled function

void Arm64Machine::door_enter() {
    // x29 (the frame) and x30 (where to return to), then the six registers we reserve
    a.stp(a64::x29, a64::x30, a64::ptr_pre(a64::sp, -64));
    a.stp(a64::x19, a64::x20, a64::ptr(a64::sp, 16));
    a.stp(a64::x21, a64::x22, a64::ptr(a64::sp, 32));
    a.stp(a64::x23, a64::x24, a64::ptr(a64::sp, 48));
    a.mov(a64::x19, a64::x0);           // thread
    a.mov(a64::x21, a64::x2);           // registers
}

void Arm64Machine::door_leave() {
    a.ldp(a64::x19, a64::x20, a64::ptr(a64::sp, 16));
    a.ldp(a64::x21, a64::x22, a64::ptr(a64::sp, 32));
    a.ldp(a64::x23, a64::x24, a64::ptr(a64::sp, 48));
    a.ldp(a64::x29, a64::x30, a64::ptr_post(a64::sp, 64));
    a.ret(a64::x30);
}

// The body calls C++, which uses x30 for ITS return address: ours is kept in the body's frame.
// 32 bytes: x29 and x30, and the slot of keep_pointer().
void Arm64Machine::body_enter() { a.stp(a64::x29, a64::x30, a64::ptr_pre(a64::sp, -32)); }

void Arm64Machine::body_return() {
    a.ldp(a64::x29, a64::x30, a64::ptr_post(a64::sp, 32));
    a.ret(a64::x30);
}

void Arm64Machine::call_label(int which) { a.bl(labels[(size_t) which]); }
void Arm64Machine::keep_pointer(PReg from) { a.str(p(from), a64::ptr(a64::sp, 16)); }
void Arm64Machine::kept_pointer(PReg to) { a.ldr(p(to), a64::ptr(a64::sp, 16)); }
void Arm64Machine::return_code(int code) { constant(a64::x0, (uint64_t) code); }

void Arm64Machine::jump_through_table(PReg table, PReg index) {
    a.ldr(CALL, a64::ptr(p(table), p(index), a64::lsl(3)));
    a.br(CALL);
}

bool Arm64Machine::failed() { return errors.failed; }

} // namespace armjit

#endif // SHARP_JIT_ARM64
