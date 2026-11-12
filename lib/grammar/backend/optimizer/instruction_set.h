//
// Sharp bytecode optimizer - instruction set description
// =======================================================
//
// One table that says, for every opcode of the Sharp VM, how it is encoded and
// what it does to the machine state. Every analysis and every optimization
// pass asks this table instead of hard-coding opcode knowledge, so:
//
//   IF YOU ADD OR CHANGE AN OPCODE (lib/core/opcode/opcode.h and the
//   interpreter in lib/runtime/virtual_machine.cpp), UPDATE ITS ROW IN
//   instruction_set.cpp. An opcode that is missing from the table makes the
//   optimizer refuse to touch any function that uses it (it never guesses).
//
// The rows mirror the interpreter (main_vm_loop). The comment beside each row
// in instruction_set.cpp is the interpreter's code for that opcode.
//
// VM STATE the table talks about
//   registers   regs[ADX..EGX], ten numbers private to the running fiber
//   locals      (fp + n), slots of the current call frame. Each slot has a
//               number part (->var) and an object part (->obj), used separately
//   stack       the operand stack above the frame (push/pop of numbers/objects)
//   ptr         task->ptr, "the object slot being looked at"; set by MOVL,
//               MOVG, MOVN..., read by GETF, PUSHOBJ, POPOBJ...
//   heap        fields and array elements: shared with other threads, so the
//               optimizer never assumes it knows what they contain
//

#ifndef SHARP_OPT_INSTRUCTION_SET_H
#define SHARP_OPT_INSTRUCTION_SET_H

#include "../../../../stdimports.h"
#include "../../../core/opcode/opcode.h"
#include "../../../core/vm_register.h"

/** How the operands are packed into the instruction word(s), see opcode_macros.h */
enum operand_layout : uint8_t {
    layout_none,    // [op]                                   no operands
    layout_d,       // [op | a]                               GET_Da
    layout_c,       // [op | a | b]                           GET_Ca, GET_Cb
    layout_b,       // [op | a | b | c]                       GET_Ba, GET_Bb, GET_Bc
    layout_raw,     // [op] [a]                               a is the whole 2nd word
    layout_d_raw,   // [op | a] [b]                           GET_Da, b is the whole 2nd word
    layout_d_c      // [op | a] [ILL | b | c]                 GET_Da, then GET_Ca/GET_Cb of the 2nd word
};

/** What an operand means */
enum operand_kind : uint8_t {
    k_none,
    k_int,          // plain number (immediate value, type id, interrupt number, flag...)
    k_reg_r,        // register that is read
    k_reg_w,        // register that is written (old value not used)
    k_reg_rw,       // register that is read and then written
    k_var_r,        // local slot (fp + n), number part is read
    k_var_w,        // local slot, number part is written
    k_var_rw,       // local slot, number part is read and then written
    k_obj_r,        // local slot, object part is read
    k_obj_w,        // local slot, object part is written
    k_slot_addr,    // local slot whose ADDRESS is taken into ptr (may then be read or written through ptr)
    k_sp_rel,       // operand stack slot relative to sp
    k_abs_slot,     // absolute stack slot (thread locals)
    k_pc,           // absolute code address inside this function (branch target)
    k_pc_rel,       // code address relative to this instruction
    k_method,       // function address (index into the method table, NOT a pc)
    k_class,        // class address
    k_string,       // string table index
    k_constant,     // constant table index
    k_field,        // field index inside an object
    k_static        // static heap address
};

/** Behaviour flags */
enum instruction_flag : uint32_t {
    f_none          = 0,
    f_jump          = 1u << 0,   // always transfers control to operand k_pc / k_pc_rel
    f_cond_jump     = 1u << 1,   // may transfer control to its target, otherwise falls through
    f_indirect_jump = 1u << 2,   // target comes from a register at run time (BRH, IFE, IFNE)
    f_terminator    = 1u << 3,   // execution never falls through to the next instruction (RET, HLT, THROW, JMP, BRH...)
    f_call          = 1u << 4,   // runs another function: registers, ptr and the heap are unknown afterwards
    f_throws        = 1u << 5,   // may raise an exception (so it cannot be removed or moved past visible effects)
    f_side_effect   = 1u << 6,   // changes something outside registers/locals/stack: heap, io, locks, threads
    f_reads_ptr     = 1u << 7,
    f_writes_ptr    = 1u << 8,
    f_reads_heap    = 1u << 9,   // result depends on a field / array element / static
    f_reads_pc      = 1u << 10,  // LOADPC: observes its own address
    f_opaque        = 1u << 11,  // effects are not described by this table at all (INT): assume everything
    f_state_check   = 1u << 12,  // the interpreter checks thread signals after it (a safepoint)
    f_allocates     = 1u << 13   // creates an object
};

/** Bit set of VM registers: bit n = register n of lib/core/vm_register.h */
typedef uint16_t register_set;
#define REG_BIT(r) ((register_set)(1u << (r)))
#define ALL_REGISTERS ((register_set)0x3ff)

struct instruction_info {
    const char *name;        // nullptr = opcode unknown to the optimizer
    operand_layout layout;
    operand_kind a, b, c;
    register_set implicitReads;   // registers read that are not operands (e.g. JE reads CMT)
    register_set implicitWrites;  // registers written that are not operands (e.g. LT writes CMT)
    int8_t popsNumbers, popsObjects;     // operand stack items consumed
    int8_t pushesNumbers, pushesObjects; // operand stack items produced
    uint32_t flags;
};

/** The table row for `opcode`. Check `known(...)` before trusting it. */
const instruction_info& instruction_info_of(uint8_t opcode);

inline bool known(const instruction_info &info) { return info.name != nullptr; }

/** Number of 32-bit words the instruction occupies (1 or 2) */
inline int words_of(const instruction_info &info) {
    return (info.layout == layout_raw || info.layout == layout_d_raw || info.layout == layout_d_c) ? 2 : 1;
}

inline bool is_register_kind(operand_kind k) { return k == k_reg_r || k == k_reg_w || k == k_reg_rw; }
inline bool reads_register(operand_kind k)   { return k == k_reg_r || k == k_reg_rw; }
inline bool writes_register(operand_kind k)  { return k == k_reg_w || k == k_reg_rw; }
inline bool is_local_kind(operand_kind k)    { return k >= k_var_r && k <= k_slot_addr; }

#endif //SHARP_OPT_INSTRUCTION_SET_H
