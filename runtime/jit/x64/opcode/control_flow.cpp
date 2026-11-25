//
// Jumps and branches. A jump to a known pc is a jump to that instruction's
// label; a jump backwards also looks at the thread's signals (the loop safe
// point, sections/slow_paths.cpp). BRH / IFE / IFNE take the pc from the adx
// register and go through the jump table (sections/dispatch.cpp).
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_control_flow() {
    switch(op) {
        case Opcode::NOP:
            return emitted;

        case Opcode::JMP:       // goto a
            return jump_to_instruction(single) ? emitted : cannot_compile;
        case Opcode::JE: {      // if(regs[CMT]) goto a
            Label skip = a.newLabel();
            a.emit(x86::Inst::kIdCmp, vm_register(CMT), Imm(0));
            a.je(skip);
            if(!jump_to_instruction(single)) return cannot_compile;
            a.bind(skip);
            return emitted;
        }
        case Opcode::JNE: {     // if(!regs[CMT]) goto a
            Label skip = a.newLabel();
            a.emit(x86::Inst::kIdCmp, vm_register(CMT), Imm(0));
            a.jne(skip);
            if(!jump_to_instruction(single)) return cannot_compile;
            a.bind(skip);
            return emitted;
        }
        case Opcode::SKIP:      // pc += a, then on to the next instruction
            if(next + single < 0 || next + single >= count || !starts[next + single]) return cannot_compile;
            a.jmp(labels[next + single]);
            return emitted;
        case Opcode::SKPE: {    // if(regs[a] != 0) pc += b;  (else the next instruction)
            if(pc + dual2 < 0 || pc + dual2 >= count || !starts[pc + dual2]) return cannot_compile;
            a.emit(x86::Inst::kIdCmp, vm_register(dual1), Imm(0));
            a.jne(labels[pc + dual2]);
            return emitted;
        }
        case Opcode::SKNE: {    // if(regs[a] == 0) pc += b;
            if(pc + dual2 < 0 || pc + dual2 >= count || !starts[pc + dual2]) return cannot_compile;
            a.emit(x86::Inst::kIdCmp, vm_register(dual1), Imm(0));
            a.je(labels[pc + dual2]);
            return emitted;
        }
        case Opcode::BRH:       // goto regs[ADX]
            load_vm_register(rax, ADX);
            jump_to_computed_instruction();
            return emitted;
        case Opcode::IFE: {     // if(regs[CMT]) goto regs[ADX]
            Label skip = a.newLabel();
            a.emit(x86::Inst::kIdCmp, vm_register(CMT), Imm(0));
            a.je(skip);
            load_vm_register(rax, ADX);
            jump_to_computed_instruction();
            a.bind(skip);
            return emitted;
        }
        case Opcode::IFNE: {
            Label skip = a.newLabel();
            a.emit(x86::Inst::kIdCmp, vm_register(CMT), Imm(0));
            a.jne(skip);
            load_vm_register(rax, ADX);
            jump_to_computed_instruction();
            a.bind(skip);
            return emitted;
        }
        case Opcode::LOADPC:    // regs[a] = this pc
            store_vm_register_constant(single, (uint64_t)pc);
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace x64jit
