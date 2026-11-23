//
// THE ARM JIT, PART 1 OF 2: THE MACHINE - what the generator asks of an ARM processor.
//
// The x86-64 JIT (jit/x64) writes machine instructions directly. ARM comes in two kinds a Sharp
// program runs on - 64-bit (a Raspberry Pi 4 or 5 with a 64-bit system) and 32-bit (every older
// Pi, and any Pi with a 32-bit system) - and they differ in more than the encoding: a VM number is
// 64 bits, which is one register on the first and TWO on the second. So the ARM JIT is cut in two:
//
//      arm_generator.h + sections/ + opcode/      WHAT a compiled function does, instruction by
//                                                 instruction - written once, in terms of the
//                                                 operations below
//      arm64/  arm32/                             HOW each of the two processors does an operation
//
// The operations are small and concrete ("load the 64 bits at fp+16 into value register 0", "add
// two value registers", "call this C++ function with these arguments"), not a general assembler:
// exactly what the generator needs. An ArmMachine is one function being written.
//
// THREE KINDS OF REGISTER
//      a POINTER register (PReg)   as wide as an address: 8 bytes on ARM64, 4 on ARM32.
//                                  Six are reserved for the whole function - THREAD, TASK, REGS,
//                                  FP, SP, PTR: the same six the x64 JIT reserves - and three
//                                  (PA, PB, PC) are scratch, free inside one bytecode instruction.
//      a VALUE register (VReg)     64 bits: a VM number as a whole number. V0, V1: scratch.
//                                  (ARM64: one register each. ARM32: two.)
//      a DECIMAL register (DReg)   a double. D0, D1: scratch.
//
// So few, because 32-bit ARM has 14 registers to give: six reserved, three pointers, two values
// that take two each, and the machine's own.
//
// A call into C++ destroys every scratch register of all three kinds; the six reserved ones stay.
// What a call returned stays where it is until result_to_...() is asked for, through these only:
// p_load_32, p_jump_if_bits, p_jump_if_zero / _not_zero, jump  (call_checked looks at the signal).
//
// THE VM REGISTERS STAY IN MEMORY. The x64 JIT keeps six of the ten VM registers in CPU
// registers ("homes") and has two modes around that. Here every VM register is read from and
// written to the fiber's register array - one mode, and far less to get wrong on two processors.
//
// LOCAL NUMBERS IN CPU REGISTERS ("homes"). The generator names the local numbers a function uses
// most (home_locals) and a machine keeps as many of them as it has registers for: a COPY, written
// together with memory, so memory is right at every moment. Arithmetic and jumps on locals are
// asked for in one piece (slot_operation, slot_jump_if ...), so that a machine works in the homes
// directly. arm64: four whole numbers and four decimals. arm32: four decimals (a whole number
// takes two registers there, and none are left).
//
#pragma once

#include "../../../../stdimports.h"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace armjit {

enum PReg { THREAD, TASK, REGS, FP, SP, PTR, PA, PB, PC };
enum VReg { V0, V1 };
enum DReg { D0, D1 };

/** whole-number operations: d = a op b */
enum VOp { V_ADD, V_SUB, V_MUL, V_AND, V_OR, V_XOR, V_SHL, V_SAR };
/** decimal operations: d = a op b */
enum DOp { D_ADD, D_SUB, D_MUL, D_DIV };
/** comparisons that give 1 or 0. Of decimals: when one is not a number, every one is false but NE. */
enum Cond { LT, GT, LE, GE, EQ, NE };
/** comparisons of pointers (and of unsigned numbers of that width) */
enum PCond { P_EQ, P_NE, P_HIGHER_OR_SAME };

/**
 * The slots of the body's frame (ArmMachine::frame_store / frame_load).
 *      FRAME_KEPT          a pointer kept across a call inside one instruction; during a call of a
 *                          function found while the code runs, that function's record
 *      FRAME_FP_BELOW_SP   during a direct call: how far our fp was below sp when we called
 *      FRAME_DIRECT        1: this body was entered by a direct call from compiled code, 0: by C++
 */
enum FrameSlot { FRAME_KEPT = 0, FRAME_FP_BELOW_SP = 1, FRAME_DIRECT = 2 };

/** One argument of a C++ function. */
struct Arg {
    enum Kind { POINTER, VALUE, CONSTANT, DECIMAL } kind;
    PReg p;         // POINTER: a pointer register (handed over as a pointer / a pointer-wide number)
    VReg v;         // VALUE: a value register, as a 64-bit whole number (the VM's Int)
    int64_t c;      // CONSTANT: a 64-bit whole number
    DReg d;         // DECIMAL: a decimal register
    static Arg pointer(PReg r) { Arg a; a.kind = POINTER; a.p = r; a.v = V0; a.c = 0; a.d = D0; return a; }
    static Arg value(VReg r) { Arg a; a.kind = VALUE; a.p = PA; a.v = r; a.c = 0; a.d = D0; return a; }
    static Arg constant(int64_t n) { Arg a; a.kind = CONSTANT; a.p = PA; a.v = V0; a.c = n; a.d = D0; return a; }
    static Arg decimal(DReg r) { Arg a; a.kind = DECIMAL; a.p = PA; a.v = V0; a.c = 0; a.d = r; return a; }
};

class ArmMachine {
public:
    virtual ~ArmMachine() {}

    // ---- places in the code
    virtual int label() = 0;                                // a new one, not placed yet
    virtual void bind(int label) = 0;                       // "here"
    virtual void jump(int label) = 0;

    // ---- 64-bit whole numbers
    virtual void v_constant(VReg to, int64_t value) = 0;
    virtual void v_load(VReg to, PReg base, int32_t offset) = 0;        // to = the 8 bytes at base+offset
    virtual void v_store(VReg from, PReg base, int32_t offset) = 0;
    virtual void v_operation(VOp op, VReg to, VReg left, VReg right) = 0;
    virtual void v_negate(VReg r) = 0;
    virtual void v_flip_sign(VReg r) = 0;                               // the top bit (a decimal's sign)
    virtual void v_extend(VReg r, int bits, bool is_signed) = 0;        // keep the low 8, 16 or 32 bits
    virtual void v_compare(VReg to, VReg left, VReg right, Cond how) = 0;   // to = (left how right) ? 1 : 0
    virtual void v_not_zero(VReg r) = 0;                                // r = (r != 0) ? 1 : 0
    virtual void v_jump_if_not_below(VReg index, PReg size, int label) = 0;    // unsigned: also when index < 0
    virtual void v_jump_if_zero(VReg r, int label) = 0;
    virtual void v_jump_if_not_zero(VReg r, int label) = 0;
    virtual void v_jump_if(VReg left, VReg right, Cond how, bool holds, int label) = 0;    // if((left how right) == holds) goto label
    virtual void v_from_pointer(VReg to, PReg from) = 0;                // widened with zeros
    virtual void v_to_pointer(PReg to, VReg from) = 0;                  // its low part (an index, an address)

    // ---- decimals
    virtual void d_constant(DReg to, double value) = 0;
    virtual void d_load(DReg to, PReg base, int32_t offset) = 0;
    virtual void d_store(DReg from, PReg base, int32_t offset) = 0;
    virtual void d_operation(DOp op, DReg to, DReg left, DReg right) = 0;
    virtual void d_square_root(DReg to, DReg from) = 0;
    virtual void d_absolute(DReg to, DReg from) = 0;                    // without its sign
    virtual bool d_round(DReg to, DReg from, bool up) = 0;              // down (floor) or up (ceil) to a whole number;
                                                                        // false: this processor has no instruction for it
    virtual void d_compare(VReg to, DReg left, DReg right, Cond how) = 0;
    virtual void d_jump_if(DReg left, DReg right, Cond how, bool holds, int label) = 0;    // (not a number: "holds" only for NE)
    virtual void d_from_whole(DReg to, VReg from) = 0;                  // (double) of a 64-bit whole number
                                                                        // (may destroy every other scratch register)

    // ---- pointers
    virtual void p_constant(PReg to, uintptr_t value) = 0;
    virtual void p_load(PReg to, PReg base, int32_t offset) = 0;        // an address stored at base+offset
    virtual void p_store(PReg from, PReg base, int32_t offset) = 0;
    virtual void p_store_object(PReg from, PReg base, int32_t offset) = 0;  // into an 8-byte object slot
    virtual void p_load_32(PReg to, PReg base, int32_t offset) = 0;     // an unsigned 32-bit number
    virtual void p_load_8(PReg to, PReg base, int32_t offset) = 0;      // an unsigned byte
    virtual void p_load_int(PReg to, PReg base, int32_t offset) = 0;    // a signed 32-bit number (a C++ `int`)
    virtual void p_store_32(PReg from, PReg base, int32_t offset) = 0;  // the low 32 bits
    virtual void p_store_8(PReg from, PReg base, int32_t offset) = 0;   // the low byte
    virtual void p_multiply(PReg r, uint32_t by) = 0;                   // r = r * by
    virtual void p_and(PReg r, uint32_t mask) = 0;                      // r = r & mask
    virtual void p_move(PReg to, PReg from) = 0;
    virtual void p_add(PReg to, PReg from, int32_t amount) = 0;
    virtual void p_add_scaled(PReg to, PReg base, PReg index, int shift) = 0;   // base + (index << shift)
    virtual void p_subtract(PReg to, PReg left, PReg right) = 0;
    virtual void p_shift_right(PReg r, int bits) = 0;                   // arithmetic
    virtual void p_jump_if_zero(PReg r, int label) = 0;
    virtual void p_jump_if_not_zero(PReg r, int label) = 0;
    virtual void p_jump_if(PReg left, PReg right, PCond how, int label) = 0;
    virtual void p_jump_if_equals(PReg r, int32_t value, int label) = 0;
    virtual void p_jump_if_bits(PReg r, uint32_t mask, bool any_set, int label) = 0;    // of its low 32 bits
    virtual void p_jump_if_machine_stack_below(PReg limit, int label) = 0;  // the processor's own stack pointer < limit

    // ---- the items of a number array: base + index * bytes
    virtual void item_load(VReg to, PReg base, PReg index, int bytes, bool is_signed) = 0;
    virtual void item_store(VReg from, PReg base, PReg index, int bytes) = 0;

    // ---- C++
    // The arguments are put where the calling convention wants them, in order. A VALUE or CONSTANT
    // argument is the VM's Int (64 bits); a POINTER argument is pointer-wide.
    virtual void call(void *function, const std::vector<Arg> &arguments) = 0;
    virtual void result_to_value(VReg to) = 0;                          // an Int result
    virtual void result_to_pointer(PReg to) = 0;                        // a pointer result, or the low part of an Int
    virtual void result_to_decimal(DReg to) = 0;

    // ---- the shape of a compiled function (sections/entry.cpp, sections/exits.cpp)
    // The C++ door:  int function(sharp_thread *thread, jit_compiled_function *fun, vm_cell *registers)
    virtual void door_enter() = 0;          // save what C++ wants back; THREAD = thread, REGS = registers
    virtual void door_leave() = 0;          // put it back and return (the return code is already set)
    // The body is entered by call_label() and left by body_return(); it keeps a slot for a pointer.
    virtual void body_enter() = 0;
    virtual void body_return() = 0;
    virtual void call_label(int label) = 0;
    // The body's frame has three slots of a pointer each (FrameSlot). They are the body's own: a
    // function called from here has its own three.
    virtual void frame_store(int slot, PReg from) = 0;
    virtual void frame_load(PReg to, int slot) = 0;
    void keep_pointer(PReg from) { frame_store(FRAME_KEPT, from); }     // across a call into C++, inside one instruction
    void kept_pointer(PReg to) { frame_load(to, FRAME_KEPT); }
    // Call the body of another compiled function through its direct door (the address is in
    // `target`). It returns here with its return code, read like the result of a C++ function.
    // Scratch registers of every kind are lost; the six reserved ones are SHARED with the callee
    // (it leaves in them what it made of them).
    virtual void call_compiled(PReg target) = 0;
    virtual void return_code(int code) = 0;         // what the function returns to C++
    virtual void jump_through_table(PReg table, PReg index) = 0;       // goto table[index]

    // ---- numbers kept in CPU registers ("homes") - a machine that has registers to spare overrides these
    // The generator says which of the function's local numbers it reads and writes most (their
    // offsets from FP; the ones used as whole numbers, the ones used as decimals; the most used
    // first), before anything is written; the machine may keep those in registers of its own.
    // What the generator promises for a function it names homes for:
    //      - a local's number is written through FP only (v_store, d_store, the slot_...() below).
    //        A store through SP is to a slot above the locals: one just pushed, or an argument.
    //      - when FP is loaded (p_load), the homes are read again: another frame ran, or the stack moved
    //      - stack_numbers_changed() is called after anything else that can have changed a local
    virtual void home_locals(const std::vector<int32_t> &, const std::vector<int32_t> &) {}
    virtual void stack_numbers_changed() {}

    // ---- arithmetic and jumps on local numbers: `to`, `left`, `right` are offsets from FP
    // Each is what its first comment line says, in one piece. The spelling here uses the scratch
    // registers; a machine with homes computes in them. (V0, V1, D0, D1 are lost either way.)
    /** fp[to] = fp[left] op fp[right] */
    virtual void slot_operation(VOp op, int32_t to, int32_t left, int32_t right) {
        v_load(V0, FP, left);
        v_load(V1, FP, right);
        v_operation(op, V0, V0, V1);
        v_store(V0, FP, to);
    }
    /** fp[to] = fp[left] op right */
    virtual void slot_operation_constant(VOp op, int32_t to, int32_t left, int64_t right) {
        v_load(V0, FP, left);
        v_constant(V1, right);
        v_operation(op, V0, V0, V1);
        v_store(V0, FP, to);
    }
    /** if((fp[left] how fp[right]) == holds) goto label */
    virtual void slot_jump_if(int32_t left, int32_t right, Cond how, bool holds, int label) {
        v_load(V0, FP, left);
        v_load(V1, FP, right);
        v_jump_if(V0, V1, how, holds, label);
    }
    /** if((fp[left] how right) == holds) goto label */
    virtual void slot_jump_if_constant(int32_t left, int64_t right, Cond how, bool holds, int label) {
        v_load(V0, FP, left);
        v_constant(V1, right);
        v_jump_if(V0, V1, how, holds, label);
    }
    /** if((fp[slot] == 0) == is_zero) goto label */
    virtual void slot_jump_if_zero(int32_t slot, bool is_zero, int label) {
        v_load(V0, FP, slot);
        if(is_zero) v_jump_if_zero(V0, label); else v_jump_if_not_zero(V0, label);
    }
    /** the same, of decimals */
    virtual void slot_decimal_operation(DOp op, int32_t to, int32_t left, int32_t right) {
        d_load(D0, FP, left);
        d_load(D1, FP, right);
        d_operation(op, D0, D0, D1);
        d_store(D0, FP, to);
    }
    virtual void slot_decimal_jump_if(int32_t left, int32_t right, Cond how, bool holds, int label) {
        d_load(D0, FP, left);
        d_load(D1, FP, right);
        d_jump_if(D0, D1, how, holds, label);
    }
    virtual void slot_decimal_jump_if_constant(int32_t left, double right, Cond how, bool holds, int label) {
        d_load(D0, FP, left);
        d_constant(D1, right);
        d_jump_if(D0, D1, how, holds, label);
    }
    /** fp[to] = (double) fp[from]   (from: a whole number) */
    virtual void slot_whole_to_decimal(int32_t to, int32_t from) {
        v_load(V0, FP, from);
        d_from_whole(D0, V0);
        d_store(D0, FP, to);
    }

    // ---- when everything is written
    virtual bool failed() = 0;
};

} // namespace armjit
