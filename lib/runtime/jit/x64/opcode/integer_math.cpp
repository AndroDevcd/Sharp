//
// Integer instructions: the plain names (ADD, LT ...). A register is one 64-bit
// integer (regs[n].i); arithmetic wraps around like int64_t in C++.
//
// The VM registers used here are mostly in CPU registers (their "homes", see
// x64_generator.h), so an instruction is one to three machine instructions
// with no memory access:
//
//      add ecx, ebx -> ecx        add r9, rsi
//
//      add ecx, ebx -> bmr        mov rax, r9
//                                 add rax, rsi
//                                 mov r11, rax
//
//      lt ecx, ebx                cmp r9, rsi          (cmt = ecx < ebx)
//                                 setl al
//                                 movzx eax, al
//                                 mov rdi, rax
//
// MOD needs the runtime (a zero divisor raises an exception). MOV8..MOVU64
// narrow a value to a small integer type and widen it back to 64 bits.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_integer_math() {
    switch(op) {
        case Opcode::MOVI:      // regs[a] = b
            store_vm_register_constant(single, (uint64_t)(int64_t)raw);
            return emitted;
        case Opcode::MOVR:      // regs[a] = regs[b]   (integer or decimal: the cell is copied)
            store_vm_register(dual1, vm_register_value(dual2, rax));
            return emitted;

        case Opcode::ADD:       // regs[c] = regs[a] + regs[b]
            integer_operation(x86::Inst::kIdAdd, triple1, triple2, triple3);
            return emitted;
        case Opcode::SUB:
            integer_operation(x86::Inst::kIdSub, triple1, triple2, triple3);
            return emitted;
        case Opcode::MUL:
            integer_operation(x86::Inst::kIdImul, triple1, triple2, triple3);
            return emitted;
        case Opcode::MOD: {     // regs[c] = vm_remainder(regs[a], regs[b])   (raises on 0)
            Label zero = a.newLabel(), done = a.newLabel();
            a.emit(x86::Inst::kIdMov, rcx, vm_register(triple2));
            a.emit(x86::Inst::kIdMov, rax, vm_register(triple1));
            divide_for_remainder(zero);
            store_vm_register(triple3, rdx);
            a.jmp(done);
            a.bind(zero);                       // a divisor of 0: the runtime raises
            prepare_runtime_call();
            a.mov(rdi, vm_register_in_memory(triple1));
            a.mov(rsi, vm_register_in_memory(triple2));
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_remainder));
            load_vm_registers();
            store_vm_register(triple3, rax);
            a.bind(done);
            return emitted;
        }

        case Opcode::IADD:      // regs[a] += b
            a.emit(x86::Inst::kIdAdd, vm_register(single), Imm(raw));
            return emitted;
        case Opcode::ISUB:
            a.emit(x86::Inst::kIdSub, vm_register(single), Imm(raw));
            return emitted;
        case Opcode::IMUL:
            a.imul(rax, vm_register_value(single, rax), Imm(raw));
            store_vm_register(single, rax);
            return emitted;
        case Opcode::IMOD:
            if(raw != 0) {                      // (0: the runtime raises, below)
                a.emit(x86::Inst::kIdMov, rax, vm_register(single));
                constant_remainder((int64_t)raw);
                store_vm_register(single, rdx);
                return emitted;
            }
            prepare_runtime_call();
            a.mov(rdi, vm_register_in_memory(single));
            a.mov(rsi, (int64_t)raw);
            call_runtime_checked(reinterpret_cast<void*>(jit_rt_remainder));
            load_vm_registers();
            store_vm_register(single, rax);
            return emitted;
        case Opcode::INC:
            a.emit(x86::Inst::kIdInc, vm_register(single));
            return emitted;
        case Opcode::DEC:
            a.emit(x86::Inst::kIdDec, vm_register(single));
            return emitted;
        case Opcode::NEG:       // regs[a] = -regs[b]
            load_vm_register(rax, dual2);
            a.neg(rax);
            store_vm_register(dual1, rax);
            return emitted;
        case Opcode::NOT:       // regs[a] = !regs[b]
            a.emit(x86::Inst::kIdCmp, vm_register(dual2), Imm(0));
            store_condition(dual1, &X64Generator::condition_equal);
            return emitted;

        case Opcode::LT:        // regs[CMT] = regs[a] < regs[b]
            integer_compare(dual1, dual2, CMT, &X64Generator::condition_less);
            return emitted;
        case Opcode::GT:
            integer_compare(dual1, dual2, CMT, &X64Generator::condition_greater);
            return emitted;
        case Opcode::LTE:
            integer_compare(dual1, dual2, CMT, &X64Generator::condition_less_or_equal);
            return emitted;
        case Opcode::GTE:
            integer_compare(dual1, dual2, CMT, &X64Generator::condition_greater_or_equal);
            return emitted;
        case Opcode::TEST:      // regs[BMR] = regs[a] == regs[b]
            integer_compare(dual1, dual2, BMR, &X64Generator::condition_equal);
            return emitted;
        case Opcode::TNE:
            integer_compare(dual1, dual2, BMR, &X64Generator::condition_not_equal);
            return emitted;
        case Opcode::CMP:       // regs[CMT] = regs[a] == b
            a.emit(x86::Inst::kIdCmp, vm_register(single), Imm(raw));
            store_condition(CMT, &X64Generator::condition_equal);
            return emitted;

        case Opcode::AND:       // regs[BMR] = regs[a] && regs[b]
            a.emit(x86::Inst::kIdCmp, vm_register(dual1), Imm(0));
            a.setne(x86::cl);
            a.emit(x86::Inst::kIdCmp, vm_register(dual2), Imm(0));
            a.setne(x86::al);
            a.and_(x86::al, x86::cl);
            a.movzx(x86::eax, x86::al);
            store_vm_register(BMR, rax);
            return emitted;
        case Opcode::UAND:      // regs[BMR] = regs[a] & regs[b]
            integer_operation(x86::Inst::kIdAnd, dual1, dual2, BMR);
            return emitted;
        case Opcode::OR:
            integer_operation(x86::Inst::kIdOr, dual1, dual2, BMR);
            return emitted;
        case Opcode::XOR:
            integer_operation(x86::Inst::kIdXor, dual1, dual2, BMR);
            return emitted;
        case Opcode::SHL:       // regs[c] = regs[a] << regs[b]   (the count goes in cl)
            load_vm_register(rcx, triple2);
            load_vm_register(rax, triple1);
            a.shl(rax, x86::cl);
            store_vm_register(triple3, rax);
            return emitted;
        case Opcode::SHR:
            load_vm_register(rcx, triple2);
            load_vm_register(rax, triple1);
            a.sar(rax, x86::cl);
            store_vm_register(triple3, rax);
            return emitted;

        // narrowing: regs[a] = (intN_t)regs[b], widened back to 64 bits
        //      C++ comparison: (int64_t)(int8_t)value
        case Opcode::MOV8:   a.movsx(rax, vm_register_value(dual2, rax).r8());    store_vm_register(dual1, rax); return emitted;
        case Opcode::MOV16:  a.movsx(rax, vm_register_value(dual2, rax).r16());   store_vm_register(dual1, rax); return emitted;
        case Opcode::MOV32:  a.movsxd(rax, vm_register_value(dual2, rax).r32());  store_vm_register(dual1, rax); return emitted;
        case Opcode::MOVU8:  a.movzx(x86::eax, vm_register_value(dual2, rax).r8());  store_vm_register(dual1, rax); return emitted;
        case Opcode::MOVU16: a.movzx(x86::eax, vm_register_value(dual2, rax).r16()); store_vm_register(dual1, rax); return emitted;
        case Opcode::MOVU32: a.mov(x86::eax, vm_register_value(dual2, rax).r32());   store_vm_register(dual1, rax); return emitted;
        case Opcode::MOV64:
        case Opcode::MOVU64:
            store_vm_register(dual1, vm_register_value(dual2, rax));
            return emitted;

        default:
            return not_mine;
    }
}

} // namespace x64jit
