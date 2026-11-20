//
// THE ARM JIT ON 32-BIT ARM (the processor of every Raspberry Pi before the 4, and of any Pi with a
// 32-bit system): the operations of arm_machine.h as machine code.
//
// The assembler library the other JITs use (AsmJit) does not know this processor, so the
// instructions are put together here, bit by bit - there are about forty kinds, each one 32-bit
// word ("ARM" instructions, not the shorter "Thumb" ones). Only instructions an ARMv6 with a
// floating point unit has are used: a Pi Zero runs this.
//
// THE REGISTERS (there are 14 to give out; a 64-bit VM number takes two)
//      r4 regs   r5 fp   r6 sp   r7 ptr   r8 task        reserved: C++ gives them back
//      thread                                            reserved too, but it lives on the stack, in
//                                                        the frame of the door and of every body: it is
//                                                        only asked for a signal, or handed to C++
//      r9 r10 r2                                         pointer scratch (PA PB PC)
//      r11:r12   lr:r3                                   value scratch (V0, V1): low half : high half
//      d4 d5                                             decimal scratch (D0 D1)
//      d8 d9 d10 d11                                     the four locals the function uses most as decimals
//      r0 r1                                             this file's own - and where C++ returns things
//      r0 .. r3, d0, d1, the stack                       arguments of C++
//
// A VM number in memory is its low half, then its high half (ARM here is little-endian).
//
// LOCAL DECIMALS THAT LIVE IN REGISTERS ("homes"). The floating point unit has registers to spare
// (and C++ hands d8 .. d15 back), the processor has none: so the locals a function uses most as
// DECIMALS get a register each, and whole numbers stay in memory. A home is a COPY that saves the
// reads: every write goes to the home and to memory, and the homes are read again whenever FP is
// loaded or the generator says a local may have changed (arm_machine.h, home_locals).
// Decimal arithmetic and jumps on locals work in the homes directly (slot_decimal_...()).
// SHARP_ARM_HOMES=0 or 1 in the environment: none (to compare).
//
// THE STACK (it must be at a multiple of 8 whenever C++ is called)
//      the door     push r4-r11, lr (36 bytes), d8-d11 (32 bytes) and 12 more:   [sp] = thread
//      the body     push lr (4 bytes) and 20 more:            [sp], [sp+4], [sp+8] = the three slots of
//                                                             frame_store(),  [sp+12] = thread
// A body gets the thread in r0 from whoever calls it - the door, or the body of another compiled
// function (call_label, call_compiled) - and keeps it in its own frame.
//
// A CALL INTO C++: every argument is first written to a block on the stack laid out as C++ expects
// (the first 16 bytes are what goes in r0..r3), then the 16 bytes are popped into r0..r3 and the
// rest is left as the stack arguments. No argument can be overwritten before it is read that way,
// wherever it comes from.
//
#include "arm32_machine.h"

#ifdef SHARP_JIT_ARM32

#include <cstring>
#include <mutex>
#include <sys/mman.h>

namespace armjit {

// registers
enum { R0 = 0, R1 = 1, R2 = 2, R3 = 3, R12 = 12, STACK = 13, LR = 14, PROGRAM = 15 };
static const int T0 = R0, T1 = R1;      // this file's own

// conditions
enum { C_EQ = 0, C_NE = 1, C_HS = 2, C_LO = 3, C_MI = 4, C_PL = 5, C_HI = 8, C_LS = 9, C_GE = 10, C_LT = 11, C_GT = 12, C_LE = 13, ALWAYS = 14 };

// operations on whole numbers ("data processing")
enum { O_AND = 0, O_EOR = 1, O_SUB = 2, O_RSB = 3, O_ADD = 4, O_ADC = 5, O_SBC = 6, O_RSC = 7,
       O_TST = 8, O_CMP = 10, O_CMN = 11, O_ORR = 12, O_MOV = 13, O_MVN = 15 };

// shifts
enum { LSL = 0, LSR = 1, ASR = 2 };

int Arm32Machine::p(PReg r) {
    switch(r) {
        case TASK: return 8;
        case REGS: return 4;
        case FP: return 5;
        case SP: return 6;
        case PTR: return 7;
        case PA: return 9;
        case PB: return 10;
        case PC: return R2;
        default: bad = true; return T0;     // THREAD is not in a register: see base_for(), p_move(), call()
    }
}

int Arm32Machine::lo(VReg r) { return r == V0 ? 11 : LR; }
int Arm32Machine::hi(VReg r) { return r == V0 ? R12 : R3; }
int Arm32Machine::d(DReg r) { return r == D0 ? 4 : 5; }

// ---------------------------------------------------------------------------------
// Instructions
// ---------------------------------------------------------------------------------

void Arm32Machine::word(int condition, uint32_t bits) {
    words.push_back(((uint32_t) condition << 28) | bits);
}

/** to = left <operation> (right shifted by a constant) */
void Arm32Machine::data(int condition, int operation, bool flags, int to, int left, int right, int shift_kind, int shift_by) {
    word(condition, ((uint32_t) operation << 21) | ((uint32_t) flags << 20) | ((uint32_t) left << 16) | ((uint32_t) to << 12)
                    | ((uint32_t) shift_by << 7) | ((uint32_t) shift_kind << 5) | (uint32_t) right);
}

/** to = right shifted by the register `by` (0 to 255: past 31 a shift gives 0, or the sign) */
void Arm32Machine::data_shifted_by_register(int condition, int operation, int to, int right, int shift_kind, int by) {
    // (operation: O_MOV, or O_ORR with `to` as the left side)
    int left = operation == O_MOV ? 0 : to;
    word(condition, ((uint32_t) operation << 21) | ((uint32_t) left << 16) | ((uint32_t) to << 12)
                    | ((uint32_t) by << 8) | ((uint32_t) shift_kind << 5) | 0x10u | (uint32_t) right);
}

/**
 * A constant inside an instruction is 8 bits turned right by an even number of places.
 * true: `value` can be written so, and `encoded` is how.
 */
static bool as_constant(uint32_t value, uint32_t &encoded) {
    for(uint32_t turn = 0; turn < 16; turn++) {
        uint32_t back = turn == 0 ? value : (value << (2 * turn)) | (value >> (32 - 2 * turn));
        if(back <= 0xFF) { encoded = (turn << 8) | back; return true; }
    }
    return false;
}

void Arm32Machine::data_constant(int condition, int operation, bool flags, int to, int left, uint32_t encoded) {
    word(condition, 0x02000000u | ((uint32_t) operation << 21) | ((uint32_t) flags << 20) | ((uint32_t) left << 16)
                    | ((uint32_t) to << 12) | encoded);
}

void Arm32Machine::move(int to, int from) {
    if(to != from) data(ALWAYS, O_MOV, false, to, 0, from);
}

/** any 32 bits into a register: one instruction when they fit, else a byte at a time */
void Arm32Machine::constant(int to, uint32_t value) {
    uint32_t encoded;
    if(as_constant(value, encoded)) { data_constant(ALWAYS, O_MOV, false, to, 0, encoded); return; }
    if(as_constant(~value, encoded)) { data_constant(ALWAYS, O_MVN, false, to, 0, encoded); return; }
    bool first = true;
    for(int shift = 0; shift < 32; shift += 8) {
        uint32_t part = value & (0xFFu << shift);
        if(part == 0) continue;
        as_constant(part, encoded);
        if(first) data_constant(ALWAYS, O_MOV, false, to, 0, encoded);
        else data_constant(ALWAYS, O_ORR, false, to, to, encoded);
        first = false;
    }
}

/** to = from + amount, for any amount (a byte of it at a time when it does not fit in one) */
void Arm32Machine::add_constant(int to, int from, int32_t amount) {
    if(amount == 0) { move(to, from); return; }
    int operation = amount > 0 ? O_ADD : O_SUB;
    uint32_t size = amount > 0 ? (uint32_t) amount : 0u - (uint32_t) amount;
    uint32_t encoded;
    if(as_constant(size, encoded)) { data_constant(ALWAYS, operation, false, to, from, encoded); return; }
    for(int shift = 0; shift < 32; shift += 8) {
        uint32_t part = size & (0xFFu << shift);
        if(part == 0) continue;
        as_constant(part, encoded);
        data_constant(ALWAYS, operation, false, to, from, encoded);
        from = to;
    }
}

/**
 * The register to use as the base of a load or store at base+offset, when the instruction takes
 * offsets up to `limit` (in_words: multiples of 4 only). When the offset is too large, or the base
 * is the thread (which is on the stack), the address is made in `temp` first.
 */
int Arm32Machine::base_for(PReg base, int32_t &offset, int limit, bool in_words, int temp) {
    int reg;
    if(base == THREAD) {
        memory(true, false, temp, STACK, thread_offset());
        reg = temp;
    } else reg = p(base);
    if(offset >= -limit && offset <= limit && (!in_words || offset % 4 == 0)) return reg;
    add_constant(temp, reg, offset);
    offset = 0;
    return temp;
}

/** a load or store of 4 bytes or of 1 (unsigned); offset: -4095 to 4095 */
void Arm32Machine::memory(bool load, bool byte, int reg, int base, int32_t offset) {
    uint32_t up = offset >= 0 ? 1u : 0u, size = (uint32_t) (offset >= 0 ? offset : -offset);
    word(ALWAYS, 0x05000000u | (up << 23) | ((uint32_t) byte << 22) | ((uint32_t) load << 20)
                 | ((uint32_t) base << 16) | ((uint32_t) reg << 12) | size);
}

/** kind 1: 2 bytes unsigned (load or store), 2: 1 byte signed (load), 3: 2 bytes signed (load); offset: -255 to 255 */
void Arm32Machine::memory_half(bool load, int kind, int reg, int base, int32_t offset) {
    uint32_t up = offset >= 0 ? 1u : 0u, size = (uint32_t) (offset >= 0 ? offset : -offset);
    word(ALWAYS, 0x01400090u | (up << 23) | ((uint32_t) load << 20) | ((uint32_t) base << 16) | ((uint32_t) reg << 12)
                 | ((uint32_t) kind << 5) | ((size >> 4) << 8) | (size & 15));
}

/** a decimal; offset: a multiple of 4, -1020 to 1020 */
void Arm32Machine::memory_decimal(bool load, int reg, int base, int32_t offset) {
    uint32_t up = offset >= 0 ? 1u : 0u, size = (uint32_t) (offset >= 0 ? offset : -offset);
    word(ALWAYS, 0x0D000B00u | (up << 23) | ((uint32_t) load << 20) | ((uint32_t) base << 16) | ((uint32_t) reg << 12) | (size >> 2));
}

/** a jump (link: a call that comes back) - its distance is filled in by install() */
void Arm32Machine::branch_to(int condition, int which, bool link) {
    jump_to_fill fill; fill.at = words.size(); fill.label = which;
    jumps.push_back(fill);
    word(condition, link ? 0x0B000000u : 0x0A000000u);
}

/** to = 1 when the condition holds, else 0 (the flags are already set) */
void Arm32Machine::one_or_zero(int condition, VReg to) {
    data_constant(ALWAYS, O_MOV, false, lo(to), 0, 0);
    data_constant(condition, O_MOV, false, lo(to), 0, 1);
    data_constant(ALWAYS, O_MOV, false, hi(to), 0, 0);
}

// ---------------------------------------------------------------------------------
// Places in the code
// ---------------------------------------------------------------------------------

int Arm32Machine::label() {
    places.push_back(-1);
    return (int) places.size() - 1;
}

void Arm32Machine::bind(int which) { places[(size_t) which] = (int) words.size(); }
void Arm32Machine::jump(int which) { branch_to(ALWAYS, which); }

// ---------------------------------------------------------------------------------
// 64-bit whole numbers: two registers each
// ---------------------------------------------------------------------------------

void Arm32Machine::v_constant(VReg to, int64_t value) {
    constant(lo(to), (uint32_t) ((uint64_t) value & 0xFFFFFFFFu));
    constant(hi(to), (uint32_t) ((uint64_t) value >> 32));
}

void Arm32Machine::v_load(VReg to, PReg base, int32_t offset) {
    int reg = base_for(base, offset, 4091, false, lo(to));
    memory(true, false, hi(to), reg, offset + 4);       // (the high half first: `reg` may be the low half's register)
    memory(true, false, lo(to), reg, offset);
}

void Arm32Machine::v_store(VReg from, PReg base, int32_t offset) {
    int home = decimal_home(base, offset);
    int reg = base_for(base, offset, 4091, false, T1);
    memory(false, false, lo(from), reg, offset);
    memory(false, false, hi(from), reg, offset + 4);
    if(home >= 0) word(ALWAYS, 0x0C400B10u | ((uint32_t) hi(from) << 16) | ((uint32_t) lo(from) << 12) | (uint32_t) home);     // vmov home, low, high
}

void Arm32Machine::v_operation(VOp op, VReg to, VReg left, VReg right) {
    int tl = lo(to), th = hi(to), ll = lo(left), lh = hi(left), rl = lo(right), rh = hi(right);
    switch(op) {
        case V_ADD:     // the low halves, then the high halves and what was carried
            data(ALWAYS, O_ADD, true, tl, ll, rl);
            data(ALWAYS, O_ADC, false, th, lh, rh);
            break;
        case V_SUB:
            data(ALWAYS, O_SUB, true, tl, ll, rl);
            data(ALWAYS, O_SBC, false, th, lh, rh);
            break;
        case V_AND:
            data(ALWAYS, O_AND, false, tl, ll, rl);
            data(ALWAYS, O_AND, false, th, lh, rh);
            break;
        case V_OR:
            data(ALWAYS, O_ORR, false, tl, ll, rl);
            data(ALWAYS, O_ORR, false, th, lh, rh);
            break;
        case V_XOR:
            data(ALWAYS, O_EOR, false, tl, ll, rl);
            data(ALWAYS, O_EOR, false, th, lh, rh);
            break;
        case V_MUL:
            // T1:T0 = left.low * right.low (all 64 bits);  T1 += left.low * right.high + left.high * right.low
            word(ALWAYS, 0x00800090u | ((uint32_t) T1 << 16) | ((uint32_t) T0 << 12) | ((uint32_t) rl << 8) | (uint32_t) ll);    // umull
            word(ALWAYS, 0x00200090u | ((uint32_t) T1 << 16) | ((uint32_t) T1 << 12) | ((uint32_t) rh << 8) | (uint32_t) ll);    // mla
            word(ALWAYS, 0x00200090u | ((uint32_t) T1 << 16) | ((uint32_t) T1 << 12) | ((uint32_t) rl << 8) | (uint32_t) lh);    // mla
            move(tl, T0);
            move(th, T1);
            break;
        case V_SHL: {
            // n = right & 63.   n < 32: high = (high << n) | (low >> (32 - n)), low <<= n
            //                   else:   high = low << (n - 32), low = 0
            // (the two cases are instructions that only run "if minus" / "if plus" after n - 32)
            uint32_t encoded;
            as_constant(63, encoded); data_constant(ALWAYS, O_AND, false, T0, rl, encoded);
            as_constant(32, encoded); data_constant(ALWAYS, O_SUB, true, T1, T0, encoded);
            data_shifted_by_register(C_PL, O_MOV, T1, ll, LSL, T1);
            data_constant(C_PL, O_MOV, false, T0, 0, 0);
            data_constant(C_MI, O_RSB, false, T1, T0, encoded);
            data_shifted_by_register(C_MI, O_MOV, T1, ll, LSR, T1);
            data_shifted_by_register(C_MI, O_ORR, T1, lh, LSL, T0);
            data_shifted_by_register(C_MI, O_MOV, T0, ll, LSL, T0);
            move(tl, T0);
            move(th, T1);
            break;
        }
        case V_SAR: {
            // n < 32: low = (low >> n) | (high << (32 - n)), high >>= n (keeping the sign)
            // else:   low = high >> (n - 32), high = its sign
            uint32_t encoded;
            as_constant(63, encoded); data_constant(ALWAYS, O_AND, false, T0, rl, encoded);
            as_constant(32, encoded); data_constant(ALWAYS, O_SUB, true, T1, T0, encoded);
            data_shifted_by_register(C_PL, O_MOV, T0, lh, ASR, T1);        // plus: low in T0, high in T1
            data(C_PL, O_MOV, false, T1, 0, lh, ASR, 31);
            data_constant(C_MI, O_RSB, false, T1, T0, encoded);
            data_shifted_by_register(C_MI, O_MOV, T1, lh, LSL, T1);
            data_shifted_by_register(C_MI, O_ORR, T1, ll, LSR, T0);        // minus: low in T1, high in T0
            data_shifted_by_register(C_MI, O_MOV, T0, lh, ASR, T0);
            data(C_MI, O_MOV, false, tl, 0, T1);
            data(C_MI, O_MOV, false, th, 0, T0);
            data(C_PL, O_MOV, false, tl, 0, T0);
            data(C_PL, O_MOV, false, th, 0, T1);
            break;
        }
    }
}

void Arm32Machine::v_negate(VReg r) {       // 0 - r
    data_constant(ALWAYS, O_RSB, true, lo(r), lo(r), 0);
    data_constant(ALWAYS, O_RSC, false, hi(r), hi(r), 0);
}

void Arm32Machine::v_flip_sign(VReg r) {
    uint32_t encoded;
    as_constant(0x80000000u, encoded);
    data_constant(ALWAYS, O_EOR, false, hi(r), hi(r), encoded);
}

void Arm32Machine::v_extend(VReg r, int bits, bool is_signed) {
    int low = lo(r), high = hi(r);
    if(bits < 32) {     // to the top of the register and back down: with the sign, or with zeros
        data(ALWAYS, O_MOV, false, low, 0, low, LSL, 32 - bits);
        data(ALWAYS, O_MOV, false, low, 0, low, is_signed ? ASR : LSR, 32 - bits);
    }
    if(is_signed) data(ALWAYS, O_MOV, false, high, 0, low, ASR, 31);
    else data_constant(ALWAYS, O_MOV, false, high, 0, 0);
}

void Arm32Machine::v_compare(VReg to, VReg left, VReg right, Cond how) {
    if(how == EQ || how == NE) {
        data(ALWAYS, O_EOR, false, T0, lo(left), lo(right));
        data(ALWAYS, O_EOR, false, T1, hi(left), hi(right));
        data(ALWAYS, O_ORR, true, T0, T0, T1);
        one_or_zero(how == EQ ? C_EQ : C_NE, to);
        return;
    }
    // left - right, 64 bits wide, only for its flags; "greater" is "less" the other way round
    VReg a = how == LT || how == GE ? left : right, b = how == LT || how == GE ? right : left;
    data(ALWAYS, O_CMP, true, 0, lo(a), lo(b));
    data(ALWAYS, O_SBC, true, T0, hi(a), hi(b));
    one_or_zero(how == LT || how == GT ? C_LT : C_GE, to);
}

void Arm32Machine::v_jump_if(VReg left, VReg right, Cond how, bool holds, int which) {
    if(how == EQ || how == NE) {
        data(ALWAYS, O_EOR, false, T0, lo(left), lo(right));
        data(ALWAYS, O_EOR, false, T1, hi(left), hi(right));
        data(ALWAYS, O_ORR, true, T0, T0, T1);
        branch_to((how == EQ) == holds ? C_EQ : C_NE, which);
        return;
    }
    // (as v_compare: left - right for its flags; "greater" is "less" the other way round)
    VReg a = how == LT || how == GE ? left : right, b = how == LT || how == GE ? right : left;
    data(ALWAYS, O_CMP, true, 0, lo(a), lo(b));
    data(ALWAYS, O_SBC, true, T0, hi(a), hi(b));
    bool less = how == LT || how == GT;
    branch_to(less == holds ? C_LT : C_GE, which);
}

void Arm32Machine::v_not_zero(VReg r) {
    data(ALWAYS, O_ORR, true, lo(r), lo(r), hi(r));
    data_constant(C_NE, O_MOV, false, lo(r), 0, 1);
    data_constant(ALWAYS, O_MOV, false, hi(r), 0, 0);
}

void Arm32Machine::v_jump_if_not_below(VReg index, PReg size, int which) {
    data_constant(ALWAYS, O_CMP, true, 0, hi(index), 0);
    branch_to(C_NE, which);                        // past 32 bits, or negative
    data(ALWAYS, O_CMP, true, 0, lo(index), p(size));
    branch_to(C_HS, which);
}

void Arm32Machine::v_jump_if_zero(VReg r, int which) {
    data(ALWAYS, O_ORR, true, T0, lo(r), hi(r));
    branch_to(C_EQ, which);
}

void Arm32Machine::v_jump_if_not_zero(VReg r, int which) {
    data(ALWAYS, O_ORR, true, T0, lo(r), hi(r));
    branch_to(C_NE, which);
}

void Arm32Machine::v_from_pointer(VReg to, PReg from) {
    move(lo(to), p(from));
    data_constant(ALWAYS, O_MOV, false, hi(to), 0, 0);
}

void Arm32Machine::v_to_pointer(PReg to, VReg from) { move(p(to), lo(from)); }

// ---------------------------------------------------------------------------------
// Decimals (the floating point unit's registers d0 .. d15)
// ---------------------------------------------------------------------------------

void Arm32Machine::d_constant(DReg to, double value) {
    uint64_t bits;
    memcpy(&bits, &value, 8);
    constant(T0, (uint32_t) (bits & 0xFFFFFFFFu));
    constant(T1, (uint32_t) (bits >> 32));
    word(ALWAYS, 0x0C400B10u | ((uint32_t) T1 << 16) | ((uint32_t) T0 << 12) | (uint32_t) d(to));      // vmov d, T0, T1
}

void Arm32Machine::d_load(DReg to, PReg base, int32_t offset) {
    int home = decimal_home(base, offset);
    if(home >= 0) { decimal_move(d(to), home); return; }
    int reg = base_for(base, offset, 1020, true, T1);
    memory_decimal(true, d(to), reg, offset);
}

void Arm32Machine::d_store(DReg from, PReg base, int32_t offset) {
    int home = decimal_home(base, offset);
    if(home >= 0) decimal_move(home, d(from));
    int reg = base_for(base, offset, 1020, true, T1);
    memory_decimal(false, d(from), reg, offset);
}

void Arm32Machine::decimal_move(int to, int from) {
    if(to != from) word(ALWAYS, 0x0EB00B40u | ((uint32_t) to << 12) | (uint32_t) from);     // vmov to, from
}

void Arm32Machine::decimal_operation(DOp op, int to, int left, int right) {
    uint32_t bits = op == D_ADD ? 0x0E300B00u : op == D_SUB ? 0x0E300B40u : op == D_MUL ? 0x0E200B00u : 0x0E800B00u;
    word(ALWAYS, bits | ((uint32_t) left << 16) | ((uint32_t) to << 12) | (uint32_t) right);
}

void Arm32Machine::d_operation(DOp op, DReg to, DReg left, DReg right) { decimal_operation(op, d(to), d(left), d(right)); }

/** (the conditions are those of arm64: with "not a number" every comparison is false but !=) */
void Arm32Machine::d_compare(VReg to, DReg left, DReg right, Cond how) {
    word(ALWAYS, 0x0EB40B40u | ((uint32_t) d(left) << 12) | (uint32_t) d(right));      // vcmp
    word(ALWAYS, 0x0EF1FA10u);                                                         // its flags to the processor's
    one_or_zero(how == LT ? C_MI : how == GT ? C_GT : how == LE ? C_LS : how == GE ? C_GE : how == EQ ? C_EQ : C_NE, to);
}

/** (the conditions of d_compare; each "does not hold" is the exact opposite, so it is taken when one is not a number) */
void Arm32Machine::d_jump_if(DReg left, DReg right, Cond how, bool holds, int which) { decimal_jump_if(d(left), d(right), how, holds, which); }

void Arm32Machine::decimal_jump_if(int left, int right, Cond how, bool holds, int which) {
    word(ALWAYS, 0x0EB40B40u | ((uint32_t) left << 12) | (uint32_t) right);            // vcmp
    word(ALWAYS, 0x0EF1FA10u);                                                         // its flags to the processor's
    int condition;
    switch(how) {
        case LT: condition = holds ? C_MI : C_PL; break;
        case GT: condition = holds ? C_GT : C_LE; break;
        case LE: condition = holds ? C_LS : C_HI; break;
        case GE: condition = holds ? C_GE : C_LT; break;
        case EQ: condition = holds ? C_EQ : C_NE; break;
        default: condition = holds ? C_NE : C_EQ; break;
    }
    branch_to(condition, which);
}

/** (the processor converts 32-bit whole numbers only: C++ does it) */
static double whole_to_decimal(int64_t value) { return (double) value; }

void Arm32Machine::d_from_whole(DReg to, VReg from) {
    move(R0, lo(from));
    move(R1, hi(from));
    constant(R12, (uint32_t) (uintptr_t) reinterpret_cast<void*>(whole_to_decimal));
    word(ALWAYS, 0x012FFF30u | (uint32_t) R12);                                         // blx r12
    word(ALWAYS, 0x0EB00B40u | ((uint32_t) d(to) << 12) | 0u);                          // vmov d(to), d0
}

// ---------------------------------------------------------------------------------
// Local decimals that live in registers (the top of this file)
// ---------------------------------------------------------------------------------

static const int DECIMAL_HOME[4] = { 8, 9, 10, 11 };

/** (a local far up the frame is left out: reading it again would need a register to build its address in) */
void Arm32Machine::home_locals(const std::vector<int32_t> &, const std::vector<int32_t> &decimal) {
    const char *text = getenv("SHARP_ARM_HOMES");
    if(text != nullptr && (text[0] == '0' || text[0] == '1')) return;
    for(int32_t offset : decimal)
        if(offset >= 0 && offset <= 1020 && decimal_homes.size() < 4) decimal_homes.push_back(offset);
}

int Arm32Machine::decimal_home(PReg base, int32_t offset) {
    if(base != FP) return -1;
    for(size_t i = 0; i < decimal_homes.size(); i++)
        if(decimal_homes[i] == offset) return DECIMAL_HOME[i];
    return -1;
}

void Arm32Machine::reload_decimal_homes() {
    for(size_t i = 0; i < decimal_homes.size(); i++)
        memory_decimal(true, DECIMAL_HOME[i], p(FP), decimal_homes[i]);
}

void Arm32Machine::stack_numbers_changed() { reload_decimal_homes(); }

/** the register a local decimal is read from: its home, or `scratch` after loading it there */
int Arm32Machine::decimal_local(int32_t offset, int scratch) {
    int home = decimal_home(FP, offset);
    if(home >= 0) return home;
    int reg = base_for(FP, offset, 1020, true, T1);
    memory_decimal(true, scratch, reg, offset);
    return scratch;
}

/** a local's new decimal is in `from`: memory gets it, and its home when it was not computed there */
void Arm32Machine::decimal_local_written(int32_t offset, int from) {
    int home = decimal_home(FP, offset);
    if(home >= 0) decimal_move(home, from);
    int reg = base_for(FP, offset, 1020, true, T1);
    memory_decimal(false, from, reg, offset);
}

void Arm32Machine::slot_decimal_operation(DOp op, int32_t to, int32_t left, int32_t right) {
    int l = decimal_local(left, d(D0)), r = decimal_local(right, d(D1));
    int home = decimal_home(FP, to), result = home >= 0 ? home : d(D0);
    decimal_operation(op, result, l, r);
    decimal_local_written(to, result);
}

void Arm32Machine::slot_decimal_jump_if(int32_t left, int32_t right, Cond how, bool holds, int which) {
    int l = decimal_local(left, d(D0)), r = decimal_local(right, d(D1));
    decimal_jump_if(l, r, how, holds, which);
}

void Arm32Machine::slot_decimal_jump_if_constant(int32_t left, double right, Cond how, bool holds, int which) {
    int l = decimal_local(left, d(D0));
    d_constant(D1, right);
    decimal_jump_if(l, d(D1), how, holds, which);
}

// ---------------------------------------------------------------------------------
// Pointers
// ---------------------------------------------------------------------------------

void Arm32Machine::p_constant(PReg to, uintptr_t value) { constant(p(to), (uint32_t) value); }

void Arm32Machine::p_load(PReg to, PReg base, int32_t offset) {
    int reg = base_for(base, offset, 4095, false, p(to));
    memory(true, false, p(to), reg, offset);
    if(to == FP) reload_decimal_homes();        // a new frame pointer: the stack moved, or other code ran
}

void Arm32Machine::p_store(PReg from, PReg base, int32_t offset) {
    int reg = base_for(base, offset, 4095, false, T1);
    memory(false, false, p(from), reg, offset);
}

/** an object slot is 8 bytes on every machine: the address, then zeros */
void Arm32Machine::p_store_object(PReg from, PReg base, int32_t offset) {
    int reg = base_for(base, offset, 4091, false, T1);
    memory(false, false, p(from), reg, offset);
    data_constant(ALWAYS, O_MOV, false, T0, 0, 0);
    memory(false, false, T0, reg, offset + 4);
}

void Arm32Machine::p_load_32(PReg to, PReg base, int32_t offset) { p_load(to, base, offset); }

void Arm32Machine::p_load_8(PReg to, PReg base, int32_t offset) {
    int reg = base_for(base, offset, 4095, false, p(to));
    memory(true, true, p(to), reg, offset);
}

void Arm32Machine::p_load_int(PReg to, PReg base, int32_t offset) { p_load(to, base, offset); }
void Arm32Machine::p_store_32(PReg from, PReg base, int32_t offset) { p_store(from, base, offset); }

void Arm32Machine::p_store_8(PReg from, PReg base, int32_t offset) {
    int reg = base_for(base, offset, 4095, false, T1);
    memory(false, true, p(from), reg, offset);
}

void Arm32Machine::p_multiply(PReg r, uint32_t by) {
    constant(T1, by);
    word(ALWAYS, 0x00000090u | ((uint32_t) p(r) << 16) | ((uint32_t) p(r) << 8) | (uint32_t) T1);    // mul r, r1, r
}

void Arm32Machine::p_and(PReg r, uint32_t mask) {
    constant(T1, mask);
    data(ALWAYS, O_AND, false, p(r), p(r), T1);
}

void Arm32Machine::p_jump_if_machine_stack_below(PReg limit, int which) {
    data(ALWAYS, O_CMP, true, 0, STACK, p(limit));
    branch_to(C_LO, which);
}

void Arm32Machine::p_move(PReg to, PReg from) {
    if(from == THREAD) memory(true, false, p(to), STACK, thread_offset());
    else move(p(to), p(from));
}

void Arm32Machine::p_add(PReg to, PReg from, int32_t amount) { add_constant(p(to), p(from), amount); }

void Arm32Machine::p_add_scaled(PReg to, PReg base, PReg index, int shift) {
    data(ALWAYS, O_ADD, false, p(to), p(base), p(index), LSL, shift);
}

void Arm32Machine::p_subtract(PReg to, PReg left, PReg right) { data(ALWAYS, O_SUB, false, p(to), p(left), p(right)); }
void Arm32Machine::p_shift_right(PReg r, int bits) { data(ALWAYS, O_MOV, false, p(r), 0, p(r), ASR, bits); }

void Arm32Machine::p_jump_if_zero(PReg r, int which) {
    data_constant(ALWAYS, O_CMP, true, 0, p(r), 0);
    branch_to(C_EQ, which);
}

void Arm32Machine::p_jump_if_not_zero(PReg r, int which) {
    data_constant(ALWAYS, O_CMP, true, 0, p(r), 0);
    branch_to(C_NE, which);
}

void Arm32Machine::p_jump_if(PReg left, PReg right, PCond how, int which) {
    data(ALWAYS, O_CMP, true, 0, p(left), p(right));
    branch_to(how == P_EQ ? C_EQ : how == P_NE ? C_NE : C_HS, which);
}

void Arm32Machine::p_jump_if_equals(PReg r, int32_t value, int which) {
    uint32_t encoded;
    if(as_constant((uint32_t) value, encoded)) data_constant(ALWAYS, O_CMP, true, 0, p(r), encoded);
    else if(as_constant(0u - (uint32_t) value, encoded)) data_constant(ALWAYS, O_CMN, true, 0, p(r), encoded);
    else { bad = true; return; }        // (r0 and r1 may hold what a call returned: no register to build it in)
    branch_to(C_EQ, which);
}

void Arm32Machine::p_jump_if_bits(PReg r, uint32_t mask, bool any_set, int which) {
    uint32_t encoded;
    if(!as_constant(mask, encoded)) { bad = true; return; }     // (as above)
    data_constant(ALWAYS, O_TST, true, 0, p(r), encoded);
    branch_to(any_set ? C_NE : C_EQ, which);
}

// ---------------------------------------------------------------------------------
// The items of a number array: the address in T1, then a load or store of the right width
// ---------------------------------------------------------------------------------

static int shift_of(int bytes) { return bytes == 1 ? 0 : bytes == 2 ? 1 : bytes == 4 ? 2 : 3; }

void Arm32Machine::item_load(VReg to, PReg base, PReg index, int bytes, bool is_signed) {
    data(ALWAYS, O_ADD, false, T1, p(base), p(index), LSL, shift_of(bytes));
    if(bytes == 8) {
        memory(true, false, lo(to), T1, 0);
        memory(true, false, hi(to), T1, 4);
        return;
    }
    if(bytes == 1) { if(is_signed) memory_half(true, 2, lo(to), T1, 0); else memory(true, true, lo(to), T1, 0); }
    else if(bytes == 2) memory_half(true, is_signed ? 3 : 1, lo(to), T1, 0);
    else memory(true, false, lo(to), T1, 0);
    if(is_signed) data(ALWAYS, O_MOV, false, hi(to), 0, lo(to), ASR, 31);
    else data_constant(ALWAYS, O_MOV, false, hi(to), 0, 0);
}

void Arm32Machine::item_store(VReg from, PReg base, PReg index, int bytes) {
    data(ALWAYS, O_ADD, false, T1, p(base), p(index), LSL, shift_of(bytes));
    if(bytes == 1) memory(false, true, lo(from), T1, 0);
    else if(bytes == 2) memory_half(false, 1, lo(from), T1, 0);
    else memory(false, false, lo(from), T1, 0);
    if(bytes == 8) memory(false, false, hi(from), T1, 4);
}

// ---------------------------------------------------------------------------------
// C++ (the "hard float" calling convention of 32-bit ARM Linux)
//
//      a pointer           the next of r0..r3, then 4 bytes of the stack
//      a 64-bit number     r0:r1 or r2:r3 (never r1:r2), then 8 bytes of the stack at a multiple of 8;
//                          once anything is on the stack, so is everything after it
//      a decimal           d0, d1 ... counted on their own
// ---------------------------------------------------------------------------------

void Arm32Machine::call(void *function, const std::vector<Arg> &arguments) {
    // where each argument goes: bytes from the start of the block (0..15 are r0..r3)
    std::vector<int> where(arguments.size(), -1);
    int next_register = 0, stack_bytes = 0, next_decimal = 0;
    for(size_t i = 0; i < arguments.size(); i++) {
        const Arg &arg = arguments[i];
        if(arg.kind == Arg::DECIMAL) continue;
        if(arg.kind == Arg::POINTER) {
            if(next_register < 4) where[i] = 4 * next_register++;
            else { where[i] = 16 + stack_bytes; stack_bytes += 4; }
        } else {
            next_register = (next_register + 1) & ~1;
            if(next_register <= 2) { where[i] = 4 * next_register; next_register += 2; }
            else {
                next_register = 4;
                stack_bytes = (stack_bytes + 7) & ~7;
                where[i] = 16 + stack_bytes;
                stack_bytes += 8;
            }
        }
    }
    stack_bytes = (stack_bytes + 7) & ~7;
    int block = 16 + stack_bytes;

    add_constant(STACK, STACK, -block);
    lowered = block;
    for(size_t i = 0; i < arguments.size(); i++) {
        const Arg &arg = arguments[i];
        switch(arg.kind) {
            case Arg::POINTER:
                if(arg.p == THREAD) {
                    memory(true, false, T0, STACK, thread_offset());
                    memory(false, false, T0, STACK, where[i]);
                } else memory(false, false, p(arg.p), STACK, where[i]);
                break;
            case Arg::VALUE:
                memory(false, false, lo(arg.v), STACK, where[i]);
                memory(false, false, hi(arg.v), STACK, where[i] + 4);
                break;
            case Arg::CONSTANT:
                constant(T0, (uint32_t) ((uint64_t) arg.c & 0xFFFFFFFFu));
                memory(false, false, T0, STACK, where[i]);
                constant(T0, (uint32_t) ((uint64_t) arg.c >> 32));
                memory(false, false, T0, STACK, where[i] + 4);
                break;
            case Arg::DECIMAL:
                word(ALWAYS, 0x0EB00B40u | ((uint32_t) next_decimal++ << 12) | (uint32_t) d(arg.d));     // vmov dN, d
                break;
        }
    }
    word(ALWAYS, 0x08BD000Fu);                                  // pop r0-r3
    lowered = stack_bytes;
    constant(R12, (uint32_t) (uintptr_t) function);
    word(ALWAYS, 0x012FFF30u | (uint32_t) R12);                 // blx r12   (C++ may be "Thumb" code: blx switches)
    if(stack_bytes != 0) add_constant(STACK, STACK, stack_bytes);
    lowered = 0;
}

void Arm32Machine::result_to_value(VReg to) {
    move(lo(to), R0);
    move(hi(to), R1);
}

void Arm32Machine::result_to_pointer(PReg to) { move(p(to), R0); }
void Arm32Machine::result_to_decimal(DReg to) { word(ALWAYS, 0x0EB00B40u | ((uint32_t) d(to) << 12) | 0u); }    // vmov d, d0

// ---------------------------------------------------------------------------------
// The shape of a compiled function
// ---------------------------------------------------------------------------------

void Arm32Machine::door_enter() {
    in_body = false;
    word(ALWAYS, 0x092D4FF0u);                  // push r4-r11, lr
    word(ALWAYS, 0x0D2D8B08u);                  // push d8-d11 (the homes of the local decimals)
    add_constant(STACK, STACK, -12);
    memory(false, false, R0, STACK, 0);         // thread
    move(4, R2);                                // registers
}

void Arm32Machine::door_leave() {
    add_constant(STACK, STACK, 12);
    word(ALWAYS, 0x0CBD8B08u);                  // pop d8-d11
    word(ALWAYS, 0x08BD8FF0u);                  // pop r4-r11, and the return address into the program counter
}

void Arm32Machine::body_enter() {
    in_body = true;                             // (everything written from here on is the body)
    word(ALWAYS, 0x092D4000u);                  // push lr
    add_constant(STACK, STACK, -20);
    memory(false, false, R0, STACK, 12);        // the thread, from whoever called
}

void Arm32Machine::body_return() {
    add_constant(STACK, STACK, 20);
    word(ALWAYS, 0x08BD8000u);                  // pop the return address into the program counter
}

void Arm32Machine::call_label(int which) {
    memory(true, false, R0, STACK, thread_offset());            // the body is handed the thread in r0
    branch_to(ALWAYS, which, true);
}

void Arm32Machine::frame_store(int slot, PReg from) { memory(false, false, p(from), STACK, slot * 4); }
void Arm32Machine::frame_load(PReg to, int slot) { memory(true, false, p(to), STACK, slot * 4); }

void Arm32Machine::call_compiled(PReg target) {
    memory(true, false, R0, STACK, thread_offset());
    word(ALWAYS, 0x012FFF30u | (uint32_t) p(target));           // blx target
}
void Arm32Machine::return_code(int code) { constant(R0, (uint32_t) code); }

void Arm32Machine::jump_through_table(PReg table, PReg index) {
    // the program counter = table[index]
    word(ALWAYS, 0x07900000u | ((uint32_t) p(table) << 16) | ((uint32_t) PROGRAM << 12) | (2u << 7) | (uint32_t) p(index));
}

// ---------------------------------------------------------------------------------
// Memory the processor may run. It is taken from the system a block at a time and handed out from
// the front; nothing is given back (compiled functions live as long as the program).
// ---------------------------------------------------------------------------------

static std::mutex memory_lock;
static uint8_t *block_at = nullptr;
static size_t block_left = 0;

static void *executable_memory(size_t bytes) {
    std::lock_guard<std::mutex> guard(memory_lock);
    bytes = (bytes + 15) & ~(size_t) 15;
    if(bytes > block_left) {
        size_t size = bytes > 256 * 1024 ? ((bytes + 4095) & ~(size_t) 4095) : 256 * 1024;
        void *got = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(got == MAP_FAILED) return nullptr;
        block_at = (uint8_t*) got;
        block_left = size;
    }
    void *result = block_at;
    block_at += bytes;
    block_left -= bytes;
    return result;
}

void *Arm32Machine::install() {
    if(bad) return nullptr;
    // a jump holds the distance in words from two instructions after itself
    for(const jump_to_fill &fill : jumps) {
        int place = places[(size_t) fill.label];
        if(place < 0) return nullptr;
        int32_t distance = place - ((int32_t) fill.at + 2);
        words[fill.at] |= (uint32_t) distance & 0x00FFFFFFu;
    }
    void *code = executable_memory(words.size() * 4);
    if(code == nullptr) return nullptr;
    memcpy(code, words.data(), words.size() * 4);
    // the processor keeps instructions it has read: tell it these are new
    __builtin___clear_cache((char*) code, (char*) code + words.size() * 4);
    return code;
}

} // namespace armjit

#endif // SHARP_JIT_ARM32
