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
// (What the JIT saves is the interpreter's fetch, decode and dispatch of every instruction; homes
// are an optimisation that can come later, inside a machine, without touching the generator.)
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
    virtual void v_from_pointer(VReg to, PReg from) = 0;                // widened with zeros
    virtual void v_to_pointer(PReg to, VReg from) = 0;                  // its low part (an index, an address)

    // ---- decimals
    virtual void d_constant(DReg to, double value) = 0;
    virtual void d_load(DReg to, PReg base, int32_t offset) = 0;
    virtual void d_store(DReg from, PReg base, int32_t offset) = 0;
    virtual void d_operation(DOp op, DReg to, DReg left, DReg right) = 0;
    virtual void d_compare(VReg to, DReg left, DReg right, Cond how) = 0;
    virtual void d_from_whole(DReg to, VReg from) = 0;                  // (double) of a 64-bit whole number
                                                                        // (may destroy every other scratch register)

    // ---- pointers
    virtual void p_constant(PReg to, uintptr_t value) = 0;
    virtual void p_load(PReg to, PReg base, int32_t offset) = 0;        // an address stored at base+offset
    virtual void p_store(PReg from, PReg base, int32_t offset) = 0;
    virtual void p_store_object(PReg from, PReg base, int32_t offset) = 0;  // into an 8-byte object slot
    virtual void p_load_32(PReg to, PReg base, int32_t offset) = 0;     // an unsigned 32-bit number
    virtual void p_load_8(PReg to, PReg base, int32_t offset) = 0;      // an unsigned byte
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
    virtual void keep_pointer(PReg from) = 0;       // into the body's slot
    virtual void kept_pointer(PReg to) = 0;
    virtual void return_code(int code) = 0;         // what the function returns to C++
    virtual void jump_through_table(PReg table, PReg index) = 0;       // goto table[index]

    // ---- when everything is written
    virtual bool failed() = 0;
};

} // namespace armjit
