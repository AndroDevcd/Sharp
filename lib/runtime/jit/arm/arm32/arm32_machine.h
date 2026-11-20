//
// The ARM JIT on 32-bit ARM: arm32_machine.cpp has the register plan and the how.
//
#pragma once

#include "../arm_machine.h"
#include "../../jit_compiler.h"

#ifdef SHARP_JIT_ARM32

namespace armjit {

class Arm32Machine : public ArmMachine {
public:
    /**
     * When the function is written: the jumps get their distances, the code is copied into memory
     * the processor may run, and its address comes back (null: it could not be done).
     */
    void *install();
    size_t code_bytes() { return words.size() * 4; }
    /** where a label ended up, in bytes from the start of the code */
    size_t offset_of(int label) { return (size_t) places[(size_t) label] * 4; }

    int label() override;
    void bind(int label) override;
    void jump(int label) override;

    void v_constant(VReg to, int64_t value) override;
    void v_load(VReg to, PReg base, int32_t offset) override;
    void v_store(VReg from, PReg base, int32_t offset) override;
    void v_operation(VOp op, VReg to, VReg left, VReg right) override;
    void v_negate(VReg r) override;
    void v_flip_sign(VReg r) override;
    void v_extend(VReg r, int bits, bool is_signed) override;
    void v_compare(VReg to, VReg left, VReg right, Cond how) override;
    void v_not_zero(VReg r) override;
    void v_jump_if_not_below(VReg index, PReg size, int label) override;
    void v_jump_if_zero(VReg r, int label) override;
    void v_jump_if_not_zero(VReg r, int label) override;
    void v_jump_if(VReg left, VReg right, Cond how, bool holds, int label) override;
    void v_from_pointer(VReg to, PReg from) override;
    void v_to_pointer(PReg to, VReg from) override;

    void d_constant(DReg to, double value) override;
    void d_load(DReg to, PReg base, int32_t offset) override;
    void d_store(DReg from, PReg base, int32_t offset) override;
    void d_operation(DOp op, DReg to, DReg left, DReg right) override;
    void d_compare(VReg to, DReg left, DReg right, Cond how) override;
    void d_jump_if(DReg left, DReg right, Cond how, bool holds, int label) override;
    void d_from_whole(DReg to, VReg from) override;

    void p_constant(PReg to, uintptr_t value) override;
    void p_load(PReg to, PReg base, int32_t offset) override;
    void p_store(PReg from, PReg base, int32_t offset) override;
    void p_store_object(PReg from, PReg base, int32_t offset) override;
    void p_load_32(PReg to, PReg base, int32_t offset) override;
    void p_load_8(PReg to, PReg base, int32_t offset) override;
    void p_load_int(PReg to, PReg base, int32_t offset) override;
    void p_store_32(PReg from, PReg base, int32_t offset) override;
    void p_store_8(PReg from, PReg base, int32_t offset) override;
    void p_multiply(PReg r, uint32_t by) override;
    void p_and(PReg r, uint32_t mask) override;
    void p_move(PReg to, PReg from) override;
    void p_add(PReg to, PReg from, int32_t amount) override;
    void p_add_scaled(PReg to, PReg base, PReg index, int shift) override;
    void p_subtract(PReg to, PReg left, PReg right) override;
    void p_shift_right(PReg r, int bits) override;
    void p_jump_if_zero(PReg r, int label) override;
    void p_jump_if_not_zero(PReg r, int label) override;
    void p_jump_if(PReg left, PReg right, PCond how, int label) override;
    void p_jump_if_equals(PReg r, int32_t value, int label) override;
    void p_jump_if_bits(PReg r, uint32_t mask, bool any_set, int label) override;
    void p_jump_if_machine_stack_below(PReg limit, int label) override;

    void item_load(VReg to, PReg base, PReg index, int bytes, bool is_signed) override;
    void item_store(VReg from, PReg base, PReg index, int bytes) override;

    void call(void *function, const std::vector<Arg> &arguments) override;
    void result_to_value(VReg to) override;
    void result_to_pointer(PReg to) override;
    void result_to_decimal(DReg to) override;

    void door_enter() override;
    void door_leave() override;
    void body_enter() override;
    void body_return() override;
    void call_label(int label) override;
    void frame_store(int slot, PReg from) override;
    void frame_load(PReg to, int slot) override;
    void call_compiled(PReg target) override;
    void return_code(int code) override;
    void jump_through_table(PReg table, PReg index) override;

    void home_locals(const std::vector<int32_t> &whole, const std::vector<int32_t> &decimal) override;
    void stack_numbers_changed() override;
    void slot_decimal_operation(DOp op, int32_t to, int32_t left, int32_t right) override;
    void slot_decimal_jump_if(int32_t left, int32_t right, Cond how, bool holds, int label) override;
    void slot_decimal_jump_if_constant(int32_t left, double right, Cond how, bool holds, int label) override;

    bool failed() override { return bad; }

private:
    struct jump_to_fill { size_t at; int label; };

    std::vector<uint32_t> words;            // the machine code: every instruction is one 32-bit word
    std::vector<int> places;                // each label: the word it is at, -1 while it is not placed
    std::vector<jump_to_fill> jumps;        // jumps whose distance is filled in by install()
    bool bad = false;                       // something this machine cannot write was asked for
    bool in_body = false;                   // the code being written runs with the body's frame on the stack
    int lowered = 0;                        // bytes the stack pointer is below its usual place (inside call())

    int p(PReg r);
    static int lo(VReg r);
    static int hi(VReg r);
    static int d(DReg r);
    int thread_offset() { return (in_body ? 12 : 0) + lowered; }

    void word(int condition, uint32_t bits);
    void data(int condition, int operation, bool flags, int to, int left, int right, int shift_kind = 0, int shift_by = 0);
    void data_shifted_by_register(int condition, int operation, int to, int right, int shift_kind, int by);
    void data_constant(int condition, int operation, bool flags, int to, int left, uint32_t encoded);
    void move(int to, int from);
    void constant(int to, uint32_t value);
    void add_constant(int to, int from, int32_t amount);
    int base_for(PReg base, int32_t &offset, int limit, bool in_words, int temp);
    void memory(bool load, bool byte, int reg, int base, int32_t offset);
    void memory_half(bool load, int kind, int reg, int base, int32_t offset);
    void memory_decimal(bool load, int reg, int base, int32_t offset);
    void branch_to(int condition, int label, bool link = false);
    void one_or_zero(int condition, VReg to);

    // ---- decimals in registers of the floating point unit, by their number (d0 .. d15)
    void decimal_move(int to, int from);
    void decimal_operation(DOp op, int to, int left, int right);
    void decimal_jump_if(int left, int right, Cond how, bool holds, int label);

    // ---- homes (arm32_machine.cpp, "LOCAL DECIMALS THAT LIVE IN REGISTERS")
    std::vector<int32_t> decimal_homes;     // the FP offsets of the locals in d8 .. d11
    int decimal_home(PReg base, int32_t offset);        // the home's register, -1: none
    void reload_decimal_homes();
    int decimal_local(int32_t offset, int scratch);     // the register a local decimal is read from
    void decimal_local_written(int32_t offset, int from);
};

} // namespace armjit

#endif // SHARP_JIT_ARM32
