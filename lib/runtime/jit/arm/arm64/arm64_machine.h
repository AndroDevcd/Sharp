//
// The ARM JIT on 64-bit ARM: arm64_machine.cpp has the register plan and the how.
//
#pragma once

#include "../arm_machine.h"
#include "../../jit_compiler.h"

#ifdef SHARP_JIT_ARM64

#include "../../../../util/jit/asmjit/src/asmjit/a64.h"

namespace armjit {

class Arm64Machine : public ArmMachine {
public:
    Arm64Machine(asmjit::a64::Assembler &assembler, JitErrorHandler &errors);

    /** the assembler's own label of one of ours (the compiler asks where it ended up) */
    asmjit::Label place(int label) { return labels[(size_t) label]; }

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
    void v_from_pointer(VReg to, PReg from) override;
    void v_to_pointer(PReg to, VReg from) override;

    void d_constant(DReg to, double value) override;
    void d_load(DReg to, PReg base, int32_t offset) override;
    void d_store(DReg from, PReg base, int32_t offset) override;
    void d_operation(DOp op, DReg to, DReg left, DReg right) override;
    void d_compare(VReg to, DReg left, DReg right, Cond how) override;
    void d_from_whole(DReg to, VReg from) override;

    void p_constant(PReg to, uintptr_t value) override;
    void p_load(PReg to, PReg base, int32_t offset) override;
    void p_store(PReg from, PReg base, int32_t offset) override;
    void p_store_object(PReg from, PReg base, int32_t offset) override;
    void p_load_32(PReg to, PReg base, int32_t offset) override;
    void p_load_8(PReg to, PReg base, int32_t offset) override;
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
    void keep_pointer(PReg from) override;
    void kept_pointer(PReg to) override;
    void return_code(int code) override;
    void jump_through_table(PReg table, PReg index) override;

    bool failed() override;

private:
    asmjit::a64::Assembler &a;
    JitErrorHandler &errors;
    std::vector<asmjit::Label> labels;

    asmjit::a64::Gp p(PReg r);
    asmjit::a64::Gp v(VReg r);
    asmjit::a64::Vec d(DReg r);
    void constant(const asmjit::a64::Gp &to, uint64_t value);
    asmjit::a64::Mem at(PReg base, int32_t offset, int size);
};

} // namespace armjit

#endif // SHARP_JIT_ARM64
