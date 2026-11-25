//
// Number fields of a class instance (GETF / PUTF) and elements of number arrays
// (ALOAD* / ASTORE*). The object is the one ptr points at.
//
//      getf ebx, slot 1           (null checks)  mov rax, [rbp]   ; the object
//                                 mov rcx, [rax]                  ; its slots
//                                 mov rdx, [rcx + 1*8]
//                                 mov rsi, rdx                    ; ebx lives in rsi
//
// The array instruction says how wide an element is and whether it is signed
// (see element_size() in memory/sharp_object.h). The index was checked by a
// CHECKLEN the compiler put in front.
//

#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

emit_result X64Generator::emit_fields_and_arrays() {
    switch(op) {
        // number fields of the object in ptr (the cell is copied whole)
        case Opcode::GETF:      // regs[a] = ptr->o.<slot b>
            load_object_or_throw();
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            a.mov(rdx, x86::qword_ptr(rcx, (int32_t)(dual2 * 8)));
            store_vm_register(dual1, rdx);
            return emitted;
        case Opcode::PUTF:      // ptr->o.<slot b> = regs[a]
            load_object_or_throw();
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            load_vm_register(rdx, dual1);
            a.mov(x86::qword_ptr(rcx, (int32_t)(dual2 * 8)), rdx);
            return emitted;

        // elements of the number array in ptr: regs[a] = array[regs[b]]
        case Opcode::ALOAD8: case Opcode::ALOADU8: case Opcode::ALOAD16: case Opcode::ALOADU16:
        case Opcode::ALOAD32: case Opcode::ALOADU32: case Opcode::ALOAD64:
            load_object_or_throw();
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, data)));
            load_vm_register(rdx, dual2);
            switch(op) {
                case Opcode::ALOAD8:   a.movsx(rax, x86::byte_ptr(rcx, rdx)); break;
                case Opcode::ALOADU8:  a.movzx(x86::eax, x86::byte_ptr(rcx, rdx)); break;
                case Opcode::ALOAD16:  a.movsx(rax, x86::word_ptr(rcx, rdx, 1)); break;
                case Opcode::ALOADU16: a.movzx(x86::eax, x86::word_ptr(rcx, rdx, 1)); break;
                case Opcode::ALOAD32:  a.movsxd(rax, x86::dword_ptr(rcx, rdx, 2)); break;
                case Opcode::ALOADU32: a.mov(x86::eax, x86::dword_ptr(rcx, rdx, 2)); break;
                default:               a.mov(rax, x86::qword_ptr(rcx, rdx, 3)); break;
            }
            store_vm_register(dual1, rax);
            return emitted;
        // array[regs[a]] = regs[b]
        case Opcode::ASTORE8: case Opcode::ASTORE16: case Opcode::ASTORE32: case Opcode::ASTORE64:
            load_object_or_throw();
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, data)));
            load_vm_register(rdx, dual1);
            load_vm_register(rax, dual2);
            switch(op) {
                case Opcode::ASTORE8:  a.mov(x86::byte_ptr(rcx, rdx), x86::al); break;
                case Opcode::ASTORE16: a.mov(x86::word_ptr(rcx, rdx, 1), x86::ax); break;
                case Opcode::ASTORE32: a.mov(x86::dword_ptr(rcx, rdx, 2), x86::eax); break;
                default:               a.mov(x86::qword_ptr(rcx, rdx, 3), rax); break;
            }
            return emitted;
        default:
            return not_mine;
    }
}

} // namespace x64jit
