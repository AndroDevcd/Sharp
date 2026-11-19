//
// Decimal instructions: the D names (DADD, DLT, DDIV, DEXP ...) and the
// conversions I2D / D2I. A register is one 64-bit double
// (regs[n].d); the SSE2 scalar instructions work on it.
//
// The homes of the VM registers are general purpose CPU registers (see
// x64_generator.h), so the bits are moved to an XMM register for the arithmetic
// and back:
//
//      dadd ecx, ebx -> bmr       movq xmm0, r9
//                                 movq xmm1, rsi
//                                 addsd xmm0, xmm1
//                                 movq r11, xmm0
//
// COMPARISONS. ucomisd sets the flags like an UNSIGNED integer comparison and
// marks "one of them is a NaN" as below-and-equal plus the parity flag. So
// `above` / `above or equal` are false for a NaN, which is what C++ gives too;
// a < b is therefore asked as b > a. == and != look at the parity flag
// (condition_decimal_equal / condition_decimal_not_equal).
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_decimal_math() {
    switch(op) {
        // ---- constants and conversions
        case Opcode::DMOVI:     // regs[a] = (double)b
            store_vm_register_constant(single, bits_of((double)raw));
            return emitted;
        case Opcode::LDC:       // regs[a] = vm.constants[b]
            store_vm_register_constant(dual1, bits_of(vm.constants[dual2]));
            return emitted;
        case Opcode::I2D:       // regs[a] = (double)regs[b]
            a.cvtsi2sd(xmm0, vm_register_value(dual2, rax));
            store_vm_register_decimal(dual1, xmm0);
            return emitted;
        case Opcode::D2I: {     // regs[a] = decimal_to_integer(regs[b])
            // cvttsd2si answers 0x8000000000000000 for a NaN and for values that
            // do not fit; decimal_to_integer() says what those become
            Label done = a.newLabel();
            vm_register_decimal(dual2, xmm0);
            a.cvttsd2si(rax, xmm0);
            a.mov(rcx, (uint64_t)0x8000000000000000ull);
            a.cmp(rax, rcx);
            a.jne(done);
            save_vm_registers();                            // rare: ask the runtime (xmm0 is the argument)
            call_runtime(reinterpret_cast<void*>(jit_rt_decimal_to_integer));
            load_vm_registers();
            a.bind(done);
            store_vm_register(dual1, rax);
            return emitted;
        }

        // ---- arithmetic
        case Opcode::DADD:      // regs[c] = regs[a] + regs[b]
            decimal_operation(x86::Inst::kIdAddsd, triple1, triple2, triple3);
            return emitted;
        case Opcode::DSUB:
            decimal_operation(x86::Inst::kIdSubsd, triple1, triple2, triple3);
            return emitted;
        case Opcode::DMUL:
            decimal_operation(x86::Inst::kIdMulsd, triple1, triple2, triple3);
            return emitted;
        case Opcode::DDIV:       // decimals only: 7 / 2 is 3.5
            decimal_operation(x86::Inst::kIdDivsd, triple1, triple2, triple3);
            return emitted;
        case Opcode::DEXP:       // regs[c] = pow(regs[a], regs[b])
            vm_register_decimal(triple1, xmm0);
            vm_register_decimal(triple2, xmm1);
            save_vm_registers();
            call_runtime(reinterpret_cast<void*>(jit_rt_power));
            load_vm_registers();
            store_vm_register_decimal(triple3, xmm0);
            return emitted;
        case Opcode::DNEG:      // regs[a] = -regs[b]: flip the sign bit
            load_vm_register(rax, dual2);
            a.btc(rax, 63);
            store_vm_register(dual1, rax);
            return emitted;
        case Opcode::DINC:
            decimal_operation_with_constant(x86::Inst::kIdAddsd, single, 1.0);
            return emitted;
        case Opcode::DDEC:
            decimal_operation_with_constant(x86::Inst::kIdSubsd, single, 1.0);
            return emitted;
        case Opcode::DIADD:     // regs[a] += (double)b
            decimal_operation_with_constant(x86::Inst::kIdAddsd, single, (double)raw);
            return emitted;
        case Opcode::DISUB:
            decimal_operation_with_constant(x86::Inst::kIdSubsd, single, (double)raw);
            return emitted;
        case Opcode::DIMUL:
            decimal_operation_with_constant(x86::Inst::kIdMulsd, single, (double)raw);
            return emitted;
        case Opcode::DIDIV:      // regs[a] /= (double)b
            decimal_operation_with_constant(x86::Inst::kIdDivsd, single, (double)raw);
            return emitted;

        // ---- comparisons (see the top of the file)
        case Opcode::DLT:       // regs[CMT] = regs[a] < regs[b]     asked as b > a
            decimal_compare(dual2, dual1);
            store_condition(CMT, &X64Generator::condition_above);
            return emitted;
        case Opcode::DGT:
            decimal_compare(dual1, dual2);
            store_condition(CMT, &X64Generator::condition_above);
            return emitted;
        case Opcode::DLTE:      // asked as b >= a
            decimal_compare(dual2, dual1);
            store_condition(CMT, &X64Generator::condition_above_or_equal);
            return emitted;
        case Opcode::DGTE:
            decimal_compare(dual1, dual2);
            store_condition(CMT, &X64Generator::condition_above_or_equal);
            return emitted;
        case Opcode::DTEST:     // regs[BMR] = regs[a] == regs[b]
            decimal_compare(dual1, dual2);
            store_condition(BMR, &X64Generator::condition_decimal_equal);
            return emitted;
        case Opcode::DTNE:
            decimal_compare(dual1, dual2);
            store_condition(BMR, &X64Generator::condition_decimal_not_equal);
            return emitted;
        case Opcode::DCMP:      // regs[CMT] = regs[a] == (double)b
            load_decimal_constant((double)raw);
            vm_register_decimal(single, xmm0);
            a.ucomisd(xmm0, xmm1);
            store_condition(CMT, &X64Generator::condition_decimal_equal);
            return emitted;
        case Opcode::DNOT:      // regs[a] = regs[b] == 0.0
            a.xorpd(xmm1, xmm1);
            vm_register_decimal(dual2, xmm0);
            a.ucomisd(xmm0, xmm1);
            store_condition(dual1, &X64Generator::condition_decimal_equal);
            return emitted;

        default:
            return not_mine;
    }
}

} // namespace x64jit
