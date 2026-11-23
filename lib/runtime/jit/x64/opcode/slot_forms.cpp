//
// THE SLOT FORMS (lib/core/opcode/opcode.h, 0xa4 .. 0xf0): one instruction that reads its operands
// from frame slots and writes its result into one.
//
//      add_s d, a, b               mov rax, [fp + a*16]            ; or the slot's home, see below
//                                  add rax, [fp + b*16]
//                                  mov [fp + d*16], rax
//
//      djlt_s a, b, 1 -> L         movsd xmm0, [fp + b*16]
//                                  ucomisd xmm0, [fp + a*16]
//                                  ja L                            ; (a < b) is (b > a)
//
//      aload64_s d, arr, i         mov rax, [fp + arr*16 + 8]  /  null?
//                                  mov rcx, [fp + i*16]
//                                  cmp rcx, size  /  outside?
//                                  mov rdx, [rax + data];  mov rax, [rdx + rcx*8];  mov [fp + d*16], rax
//
// None of them touches a VM register, so the two modes of x64_generator.h do not come into it:
// they are written in register mode like every instruction and leave the VM registers' homes alone.
//
// THE SLOTS A FUNCTION USES MOST HAVE HOMES of their own (x64_generator.cpp, "LOCAL NUMBERS IN CPU
// REGISTERS"): with `i` in r8 and `total` in xmm2,
//
//      dadd_s total, total, x      addsd xmm2, [fp + x*16]
//                                  movsd [fp + total*16], xmm2     ; memory is kept right, always
//
// Every operand is taken through slot_in_register / slot_operand / store_slot and their decimal
// twins, which know about the homes.
//
#include "../x64_generator.h"

using namespace asmjit;

namespace x64jit {

/** jump to `to` when (fp[a] <how> fp[b]) is `when`. how: 0 lt, 1 gt, 2 le, 3 ge, 4 eq, 5 ne */
void X64Generator::slot_jump_integers(int how, Int left, Int right, bool when, const Label &to) {
    x86::Gp value = slot_in_register(left, rax);
    a.emit(x86::Inst::kIdCmp, value, slot_operand(right));
    jump_after_integer_compare(how, when, to);
}

/** ... when (fp[slot] <how> constant) is `when` */
void X64Generator::slot_jump_integer_constant(int how, Int slot, Int constant, bool when, const Label &to) {
    a.emit(x86::Inst::kIdCmp, slot_operand(slot), Imm(constant));
    jump_after_integer_compare(how, when, to);
}

void X64Generator::jump_after_integer_compare(int how, bool when, const Label &to) {
    // the condition to jump on: `how` itself, or its opposite
    static const int opposite[6] = { 3, 2, 1, 0, 5, 4 };
    switch(when ? how : opposite[how]) {
        case 0: a.jl(to); break;
        case 1: a.jg(to); break;
        case 2: a.jle(to); break;
        case 3: a.jge(to); break;
        case 4: a.je(to); break;
        default: a.jne(to); break;
    }
}

/**
 * The same for decimals. A comparison with "not a number" is false, except != which is true:
 * so "not less" is NOT "greater or equal", and each case is written out. ucomisd sets
 * CF = ZF = PF = 1 for "not a number"; ja / jae are false then, jb / jbe true.
 */
void X64Generator::slot_jump_decimals(int how, Int left, Int right, bool when, const Label &to) {
    // a < b is asked as b > a, and a <= b as b >= a: `ja` and `jae` are the tests that are false for NaN
    bool swapped = how == 0 || how == 2;
    x86::Xmm value = slot_in_decimal_register(swapped ? right : left, xmm0);
    a.emit(x86::Inst::kIdUcomisd, value, slot_decimal_operand(swapped ? left : right));
    jump_after_decimal_compare(how, when, to);
}

/** ... when (fp[slot] <how> constant) is `when`, the constant a small whole number */
void X64Generator::slot_jump_decimal_constant(int how, Int slot, Int constant, bool when, const Label &to) {
    load_decimal_constant((double)constant);            // xmm1
    if(how == 0 || how == 2) a.emit(x86::Inst::kIdUcomisd, xmm1, slot_decimal_operand(slot));     // swapped, as above
    else a.ucomisd(slot_in_decimal_register(slot, xmm0), xmm1);
    jump_after_decimal_compare(how, when, to);
}

/** the jump after `ucomisd` whose operands were swapped for < and <= (slot_jump_decimals says why) */
void X64Generator::jump_after_decimal_compare(int how, bool when, const Label &to) {
    switch(how) {
        case 0: case 1:         // strictly greater (after the swap)
            if(when) a.ja(to); else a.jbe(to);
            break;
        case 2: case 3:         // greater or equal
            if(when) a.jae(to); else a.jb(to);
            break;
        case 4: {               // equal: ZF and not PF
            if(when) { Label no = a.newLabel(); a.jp(no); a.je(to); a.bind(no); }
            else { a.jp(to); a.jne(to); }
            break;
        }
        default: {              // not equal: not ZF, or PF
            if(when) { a.jp(to); a.jne(to); }
            else { Label no = a.newLabel(); a.jp(no); a.je(to); a.bind(no); }
            break;
        }
    }
}

/**
 * rax = the object in the slot, or the null exception. The interpreter puts the object pointer on
 * the slot first (as `movl` does); here that is done only on the way to an exception - the one
 * place that reads it (sections/slow_paths.cpp) - and not on every access.
 */
void X64Generator::slot_object_or_throw(Int slot) {
    a.mov(rax, local_object(slot));
    a.test(rax, rax);
    a.jz(request_slow_path(slow_null, pc));
    slowPaths.back().objectSlot = slot;
}

/** ... and rcx = the index in slot `index`, inside the array or the bounds exception */
void X64Generator::slot_element_or_throw(Int array, Int index) {
    slot_object_or_throw(array);
    a.emit(x86::Inst::kIdMov, rcx, slot_operand(index));
    a.mov(x86::edx, x86::dword_ptr(rax, offsetof(sharp_object, size)));
    a.cmp(rcx, rdx);                    // unsigned: a negative index is a huge one
    a.jae(request_slow_path(slow_bounds, pc));
    slowPaths.back().objectSlot = array;
}

emit_result X64Generator::emit_slot_forms() {
    switch(op) {
        // ---- whole numbers: fp[a] = fp[b] op fp[c]
        case Opcode::ADD_S: case Opcode::SUB_S: case Opcode::MUL_S:
        case Opcode::AND_S: case Opcode::OR_S: case Opcode::XOR_S: {
            InstId inst = op == Opcode::ADD_S ? x86::Inst::kIdAdd : op == Opcode::SUB_S ? x86::Inst::kIdSub
                        : op == Opcode::MUL_S ? x86::Inst::kIdImul : op == Opcode::AND_S ? x86::Inst::kIdAnd
                        : op == Opcode::OR_S ? x86::Inst::kIdOr : x86::Inst::kIdXor;
            // in the result's home when it has one; `i = i + n` with i at home has its left side there already
            x86::Gp result = whole_result_register(triple1, triple3);
            bool inPlace = result.id() != rax.id() && triple1 == triple2;
            if(!inPlace) a.emit(x86::Inst::kIdMov, result, slot_operand(triple2));
            a.emit(inst, result, slot_operand(triple3));
            store_slot(triple1, result);
            return emitted;
        }
        case Opcode::SHL_S: case Opcode::SHR_S: {   // (the count modulo 64, as the register forms)
            a.emit(x86::Inst::kIdMov, rcx, slot_operand(triple3));
            x86::Gp result = whole_result_register(triple1);
            bool inPlace = result.id() != rax.id() && triple1 == triple2;
            if(!inPlace) a.emit(x86::Inst::kIdMov, result, slot_operand(triple2));
            if(op == Opcode::SHL_S) a.shl(result, x86::cl); else a.sar(result, x86::cl);
            store_slot(triple1, result);
            return emitted;
        }

        // ---- decimals
        case Opcode::DADD_S: case Opcode::DSUB_S: case Opcode::DMUL_S: case Opcode::DDIV_S: {
            InstId inst = op == Opcode::DADD_S ? x86::Inst::kIdAddsd : op == Opcode::DSUB_S ? x86::Inst::kIdSubsd
                        : op == Opcode::DMUL_S ? x86::Inst::kIdMulsd : x86::Inst::kIdDivsd;
            x86::Xmm result = decimal_result_register(triple1, triple3);
            bool inPlace = result.id() != xmm0.id() && triple1 == triple2;
            if(!inPlace) {
                x86::Xmm left = slot_in_decimal_register(triple2, result);
                if(left.id() != result.id()) a.movaps(result, left);
            }
            a.emit(inst, result, slot_decimal_operand(triple3));
            store_slot_decimal(triple1, result);
            return emitted;
        }
        case Opcode::DMATH_S: {     // fp[a] = math function c of fp[b]
            x86::Xmm result = decimal_result_register(triple1);
            static const bool rounds = CpuInfo::host().hasFeature(CpuFeatures::X86::kSSE4_1);
            if(triple3 == 0xe) {                                        // sqrt: the processor's own
                a.sqrtsd(result, slot_in_decimal_register(triple2, xmm0));
            } else if((triple3 == 0x10 || triple3 == 0xf) && rounds) {  // floor, ceil: roundsd down (9) or up (10)
                a.roundsd(result, slot_in_decimal_register(triple2, xmm0), triple3 == 0x10 ? 9 : 10);
            } else if(triple3 == 0x11) {                                // abs: the sign bit cleared
                a.mov(rax, 0x7fffffffffffffffull);
                a.movq(xmm1, rax);
                x86::Xmm from = slot_in_decimal_register(triple2, xmm0);
                if(result.id() != from.id()) a.movaps(result, from);
                a.andpd(result, xmm1);
            } else {                                                    // the C library's function
                x86::Xmm from = slot_in_decimal_register(triple2, xmm0);
                if(from.id() != xmm0.id()) a.movaps(xmm0, from);
                a.xorps(xmm1, xmm1);
                save_vm_registers();
                a.mov(rax, bits_of((double)triple3));
                a.movq(x86::xmm2, rax);
                call_runtime(reinterpret_cast<void*>(jit_rt_math));
                load_vm_registers();
                result = xmm0;
            }
            store_slot_decimal(triple1, result);
            return emitted;
        }
        case Opcode::I2D_S: {
            x86::Xmm result = decimal_result_register(triple1);
            a.xorps(result, result);                // (cvtsi2sd keeps the upper half: do not wait for it)
            a.emit(x86::Inst::kIdCvtsi2sd, result, slot_operand(triple2));
            store_slot_decimal(triple1, result);
            return emitted;
        }
        case Opcode::D2I_S: {
            // as D2I (opcode/decimal_math.cpp): the runtime answers for what the CPU cannot convert
            Label done = a.newLabel();
            x86::Xmm value = slot_in_decimal_register(triple2, xmm0);
            if(value.id() != xmm0.id()) a.movaps(xmm0, value);
            a.cvttsd2si(rax, xmm0);
            a.cmp(rax, 1);                          // only 0x8000000000000000 - 1 overflows: the CPU's "cannot convert"
            a.jno(done);
            save_vm_registers();
            call_runtime(reinterpret_cast<void*>(jit_rt_decimal_to_integer));
            load_vm_registers();
            a.bind(done);
            store_slot(triple1, rax);
            return emitted;
        }

        // ---- compare and jump: if((fp[a] ? fp[b]) == c) goto <second word>
        case Opcode::JLT_S: case Opcode::JGT_S: case Opcode::JLE_S: case Opcode::JGE_S:
        case Opcode::JEQ_S: case Opcode::JNE_S:
        case Opcode::DJLT_S: case Opcode::DJGT_S: case Opcode::DJLE_S: case Opcode::DJGE_S:
        case Opcode::DJEQ_S: case Opcode::DJNE_S: {
            if(!before_conditional_jump(raw)) return cannot_compile;
            bool sense = triple3 != 0;
            if(op >= Opcode::DJLT_S) slot_jump_decimals(op - Opcode::DJLT_S, triple1, triple2, sense, labels[raw]);
            else slot_jump_integers(op - Opcode::JLT_S, triple1, triple2, sense, labels[raw]);
            return emitted;
        }

        // ---- elements of a number array
        case Opcode::ALOAD8_S: case Opcode::ALOADU8_S: case Opcode::ALOAD16_S: case Opcode::ALOADU16_S:
        case Opcode::ALOAD32_S: case Opcode::ALOADU32_S: case Opcode::ALOAD64_S:
        {
            slot_element_or_throw(triple2, triple3);
            a.mov(rdx, x86::qword_ptr(rax, offsetof(sharp_object, data)));
            if(op == Opcode::ALOAD64_S && decimal_home(triple1) >= 0) {
                // an element of a decimal array, straight into the slot's decimal home
                x86::Xmm home = decimal_home_register(decimal_home(triple1));
                a.movsd(home, x86::qword_ptr(rdx, rcx, 3));
                store_slot_decimal(triple1, home);
                return emitted;
            }
            x86::Gp result = whole_result_register(triple1);
            switch(op) {
                case Opcode::ALOAD8_S:   a.movsx(result, x86::byte_ptr(rdx, rcx)); break;
                case Opcode::ALOADU8_S:  a.movzx(result.r32(), x86::byte_ptr(rdx, rcx)); break;
                case Opcode::ALOAD16_S:  a.movsx(result, x86::word_ptr(rdx, rcx, 1)); break;
                case Opcode::ALOADU16_S: a.movzx(result.r32(), x86::word_ptr(rdx, rcx, 1)); break;
                case Opcode::ALOAD32_S:  a.movsxd(result, x86::dword_ptr(rdx, rcx, 2)); break;
                case Opcode::ALOADU32_S: a.mov(result.r32(), x86::dword_ptr(rdx, rcx, 2)); break;
                default:                 a.mov(result, x86::qword_ptr(rdx, rcx, 3)); break;
            }
            store_slot(triple1, result);
            return emitted;
        }
        case Opcode::ASTORE8_S: case Opcode::ASTORE16_S: case Opcode::ASTORE32_S: case Opcode::ASTORE64_S:
            slot_element_or_throw(triple1, triple2);
            a.mov(rdx, x86::qword_ptr(rax, offsetof(sharp_object, data)));
            if(op == Opcode::ASTORE64_S && decimal_home(triple3) >= 0) {
                // a decimal that is at home in a register: from there, straight into the array
                a.movsd(x86::qword_ptr(rdx, rcx, 3), decimal_home_register(decimal_home(triple3)));
                return emitted;
            }
            {
                x86::Gp value = slot_in_register(triple3, rax);     // its home, or rax
                switch(op) {
                    case Opcode::ASTORE8_S:  a.mov(x86::byte_ptr(rdx, rcx), value.r8()); break;
                    case Opcode::ASTORE16_S: a.mov(x86::word_ptr(rdx, rcx, 1), value.r16()); break;
                    case Opcode::ASTORE32_S: a.mov(x86::dword_ptr(rdx, rcx, 2), value.r32()); break;
                    default:                 a.mov(x86::qword_ptr(rdx, rcx, 3), value); break;
                }
            }
            return emitted;

        // ---- number fields
        case Opcode::GETF_S:    // fp[a] = (fp[b].obj).<field c>
            slot_object_or_throw(triple2);
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            if(decimal_home(triple1) >= 0) {        // a decimal field, straight into the slot's decimal home
                x86::Xmm home = decimal_home_register(decimal_home(triple1));
                a.movsd(home, x86::qword_ptr(rcx, (int32_t)(triple3 * 8)));
                store_slot_decimal(triple1, home);
                return emitted;
            }
            {
                x86::Gp result = whole_result_register(triple1);
                a.mov(result, x86::qword_ptr(rcx, (int32_t)(triple3 * 8)));
                store_slot(triple1, result);
            }
            return emitted;
        case Opcode::PUTF_S:    // (fp[a].obj).<field b> = fp[c]
            slot_object_or_throw(triple1);
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            a.mov(rdx, local_number(triple3));
            a.mov(x86::qword_ptr(rcx, (int32_t)(triple2 * 8)), rdx);
            return emitted;
        case Opcode::SIZEOF_S: {    // fp[a] = the length of fp[b].obj, 0 for null
            Label done = a.newLabel();
            a.mov(rcx, local_object(triple2));
            a.xor_(x86::eax, x86::eax);
            a.test(rcx, rcx);
            a.jz(done);
            a.mov(x86::eax, x86::dword_ptr(rcx, offsetof(sharp_object, size)));
            a.bind(done);
            store_slot(triple1, rax);
            return emitted;
        }

        // ---- references out of an array and out of an object, into a slot (a stack slot is a root
        // of the collector: storing into it needs no write barrier, as PUSHL and POPL do not)
        case Opcode::AGETOBJ_S:
            slot_element_or_throw(triple2, triple3);
            a.mov(rdx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            a.mov(rax, x86::qword_ptr(rdx, rcx, 3));
            a.mov(local_object(triple1), rax);
            return emitted;
        case Opcode::GETOBJ_S:
            slot_object_or_throw(triple2);
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            a.mov(rdx, x86::qword_ptr(rcx, (int32_t)(triple3 * 8)));
            a.mov(local_object(triple1), rdx);
            return emitted;

        // ================================================================ the second group
        // ---- jump on a slot: if(fp[a] != 0) / if(fp[a] == 0) goto <second word>
        case Opcode::JNZ_S: case Opcode::JZ_S: {
            if(!before_conditional_jump(raw)) return cannot_compile;
            int home = whole_home(single);
            if(home >= 0) a.test(whole_home_register(home), whole_home_register(home));
            else a.cmp(local_number(single), 0);
            if(op == Opcode::JNZ_S) a.jne(labels[raw]); else a.je(labels[raw]);
            return emitted;
        }
        case Opcode::JNULL_S: {     // if((fp[a].obj is null) == c) goto <second word>
            if(!before_conditional_jump(raw)) return cannot_compile;
            a.cmp(local_object(triple1), 0);
            if(triple3 != 0) a.je(labels[raw]); else a.jne(labels[raw]);
            return emitted;
        }
        case Opcode::ISNULL_S:      // fp[a] = fp[b].obj is null
            a.xor_(x86::eax, x86::eax);
            a.cmp(local_object(triple2), 0);
            a.sete(x86::al);
            store_slot(triple1, rax);
            return emitted;
        case Opcode::NOT_S:         // fp[a] = !fp[b]
            a.xor_(x86::eax, x86::eax);
            a.emit(x86::Inst::kIdCmp, slot_operand(triple2), Imm(0));
            a.sete(x86::al);
            store_slot(triple1, rax);
            return emitted;

        // ---- fp[a] = fp[b] cut to a narrower whole number (read from memory, which is always right:
        // the low bytes come first on this processor)
        case Opcode::MOV8_S:   a.movsx(rax, x86::byte_ptr(FP, (int32_t)(triple2 * 16)));  store_slot(triple1, rax); return emitted;
        case Opcode::MOV16_S:  a.movsx(rax, x86::word_ptr(FP, (int32_t)(triple2 * 16)));  store_slot(triple1, rax); return emitted;
        case Opcode::MOV32_S:  a.movsxd(rax, x86::dword_ptr(FP, (int32_t)(triple2 * 16))); store_slot(triple1, rax); return emitted;
        case Opcode::MOVU8_S:  a.movzx(x86::eax, x86::byte_ptr(FP, (int32_t)(triple2 * 16))); store_slot(triple1, rax); return emitted;
        case Opcode::MOVU16_S: a.movzx(x86::eax, x86::word_ptr(FP, (int32_t)(triple2 * 16))); store_slot(triple1, rax); return emitted;
        case Opcode::MOVU32_S: a.mov(x86::eax, x86::dword_ptr(FP, (int32_t)(triple2 * 16)));  store_slot(triple1, rax); return emitted;

        // ---- what a function hands back: into fp[0], where its caller finds it
        case Opcode::RETURNVAL_S:
            store_slot(0, slot_in_register(single, rax));
            return emitted;
        case Opcode::RETURNOBJ_S:
            a.mov(rax, local_object(single));
            a.mov(local_object(0), rax);
            return emitted;

        // ---- fp[a] = fp[b] op <second word>
        case Opcode::IADD_S: case Opcode::ISUB_S: case Opcode::IMUL_S: {
            int home = whole_home(triple1);
            if(home >= 0 && triple1 == triple2) {
                x86::Gp at = whole_home_register(home);
                if(op == Opcode::IADD_S) a.add(at, Imm(raw)); else if(op == Opcode::ISUB_S) a.sub(at, Imm(raw));
                else a.imul(at, at, Imm(raw));
                a.mov(local_number(triple1), at);
                return emitted;
            }
            if(op == Opcode::IMUL_S) a.emit(x86::Inst::kIdImul, rax, slot_operand(triple2), Imm(raw));
            else {
                a.emit(x86::Inst::kIdMov, rax, slot_operand(triple2));
                if(op == Opcode::IADD_S) a.add(rax, Imm(raw)); else a.sub(rax, Imm(raw));
            }
            store_slot(triple1, rax);
            return emitted;
        }

        // ---- (fp[a].obj).<reference field b> = fp[c].obj: what movl, movn and popobj do together.
        // ptr and the fiber's ptrOwner are left as they leave them: the write barrier's slow path
        // (sections/slow_paths.cpp) reads both.
        case Opcode::PUTOBJ_S: {
            slot_object_or_throw(triple1);                                  // rax = the owner
            a.mov(fiber_field(offsetof(fiber, ptrOwner)), rax);
            a.mov(rcx, x86::qword_ptr(rax, offsetof(sharp_object, node)));
            a.lea(PTR, x86::qword_ptr(rcx, (int32_t)(triple2 * 8)));
            a.mov(rdx, local_object(triple3));
            a.mov(x86::qword_ptr(PTR), rdx);
            // write_barrier(owner, value): only a YOUNG value stored into an OLD object that is not
            // remembered yet goes to the runtime (as POPOBJ, opcode/objects.cpp)
            Label done = a.newLabel();
            Label remember = request_slow_path(slow_barrier, pc, done);
            a.test(rdx, rdx);
            a.jz(done);
            a.test(x86::dword_ptr(rdx, offsetof(sharp_object, info)), 0x30000000);
            a.jnz(done);                                // the value is not young
            a.test(x86::dword_ptr(rax, offsetof(sharp_object, info)), 0x30000000);
            a.jz(done);                                 // the owner is young
            a.cmp(x86::byte_ptr(rax, offsetof(sharp_object, remembered)), 0);
            a.je(remember);
            a.bind(done);
            return emitted;
        }
        case Opcode::MOVOBJ_S:      // fp[a].obj = fp[b].obj
            a.mov(rax, local_object(triple2));
            a.mov(local_object(triple1), rax);
            return emitted;

        // ================================================================ the third group: a constant operand
        // ---- fp[a] = fp[b] op <second word>
        case Opcode::IAND_S: case Opcode::IOR_S: case Opcode::IXOR_S: case Opcode::ISHL_S: case Opcode::ISHR_S: {
            InstId inst = op == Opcode::IAND_S ? x86::Inst::kIdAnd : op == Opcode::IOR_S ? x86::Inst::kIdOr
                        : op == Opcode::IXOR_S ? x86::Inst::kIdXor : op == Opcode::ISHL_S ? x86::Inst::kIdShl : x86::Inst::kIdSar;
            Imm constant = op == Opcode::ISHL_S || op == Opcode::ISHR_S ? Imm(raw & 63) : Imm(raw);
            int home = whole_home(triple1);
            if(home >= 0 && triple1 == triple2) {
                a.emit(inst, whole_home_register(home), constant);
                a.mov(local_number(triple1), whole_home_register(home));
                return emitted;
            }
            a.emit(x86::Inst::kIdMov, rax, slot_operand(triple2));
            a.emit(inst, rax, constant);
            store_slot(triple1, rax);
            return emitted;
        }
        // ---- compare with a constant and jump: if((fp[a] ? b) == c) goto <second word>
        case Opcode::JLT_K: case Opcode::JGT_K: case Opcode::JLE_K: case Opcode::JGE_K:
        case Opcode::JEQ_K: case Opcode::JNE_K:
        case Opcode::DJLT_K: case Opcode::DJGT_K: case Opcode::DJLE_K: case Opcode::DJGE_K:
        case Opcode::DJEQ_K: case Opcode::DJNE_K: {
            if(!before_conditional_jump(raw)) return cannot_compile;
            bool sense = triple3 != 0;
            if(op >= Opcode::DJLT_K) slot_jump_decimal_constant(op - Opcode::DJLT_K, triple1, triple2, sense, labels[raw]);
            else slot_jump_integer_constant(op - Opcode::JLT_K, triple1, triple2, sense, labels[raw]);
            return emitted;
        }
        default:
            return not_mine;
    }
}

} // namespace x64jit
