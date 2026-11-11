//
// Sharp bytecode optimizer - the editable form of one function
// ============================================================
//
// The code generator stores a finished function as a flat list of 32-bit words
// (code_info::code) in which every code address - branch targets, the try/catch
// table, the line table - is a plain number. Removing or inserting a single
// instruction would silently break all of them.
//
// function_ir is the same function in a form that is safe to edit:
//
//   * every instruction is one ir_instruction with decoded operands and a
//     unique id that never changes
//   * everything that refers to a place in the code refers to an instruction ID,
//     not to an address
//
// decode_function() builds it, the passes edit it, encode_function() writes the
// words back and recomputes every address from the ids.
//
// WHAT COUNTS AS "A PLACE IN THE CODE" (all of these are remapped):
//   - the target of JMP / JE / JNE (and the pc-relative SKIP / SKPE / SKNE)
//   - a MOVI whose value is a code address. The generator emits these to
//     remember where to continue after a `finally` block (break / continue /
//     return inside try). In the bytecode they look like ordinary numbers, so
//     the generator tags them: code_info::labelOperands.
//   - the line table (line_info::start_pc)
//   - the try/catch table (try/block ranges, catch handlers, finally ranges)
//
// A reference to an instruction that gets removed moves to the instruction that
// follows it, exactly as if a label had been sitting there.
//
// FUNCTIONS THAT ARE LEFT ALONE (decode_function returns false, see `failure`):
//   - functions containing hand written asm { } (code_info::hasAsm): asm can
//     compute code addresses at run time, which cannot be tracked
//   - anything unexpected: an opcode missing from instruction_set.cpp, a branch
//     into the middle of an instruction, LOADPC...
//   The optimizer never guesses. A function it does not fully understand is
//   emitted exactly as the generator produced it.
//

#ifndef SHARP_OPT_FUNCTION_IR_H
#define SHARP_OPT_FUNCTION_IR_H

#include "instruction_set.h"
#include "../types/types.h"
#include "../finalizer/generation/code/code_info.h"
#include <vector>
#include <string>

/** "no instruction" */
#define IR_NONE (-1)
/** the position one past the last instruction (used by ranges that end at the end of the function) */
#define IR_END (-2)

struct ir_instruction {
    int id;                 // unique in the function, stable across edits
    uint8_t op;
    int32_t a, b, c;        // operands, meaning given by instruction_info_of(op)
    opcode_instr raw[2];    // the words it was decoded from, reused when nothing changed
    bool modified;          // operands were changed (or it is new): raw is stale, re-encode
    int target;             // instruction id its code-address operand refers to, IR_NONE if it has none
    bool pcConstant;        // a MOVI whose value is a code address (see header comment)
};

struct ir_line_entry {
    int at;                 // instruction id the line starts at
    Int line;
};

struct ir_try_entry {
    int tryStart, tryEnd;       // ids, IR_NONE when the generator left the field unset (-1)
    int blockStart, blockEnd;
    std::vector<int> handlers;  // one per catch_data, same order
    bool hasFinally;
    int finallyStart, finallyEnd;
};

struct function_ir {
    sharp_function *fn;
    code_info *ci;
    std::vector<ir_instruction> code;       // in execution order
    std::vector<ir_line_entry> lines;       // same order as ci->lineTable
    std::vector<ir_try_entry> tries;        // same order as ci->tryCatchTable
    std::vector<int> forward;               // forward[id] = id that replaced a removed instruction, else IR_NONE
    int nextId;
    std::string failure;                    // why the function cannot be optimized ("" = it can)

    // Number of frame slots (fp+0 .. fp+frameSlots-1) the function uses. Starts
    // as code_info::stackSize. A pass that needs extra locals (inlining puts
    // the callee's locals in the caller's frame) raises it, and
    // encode_function() then updates stackSize and frameStackOffset.
    int frameSlots;
    bool changed;                           // a pass edited the function since it was decoded

    function_ir() : fn(nullptr), ci(nullptr), nextId(0), frameSlots(0), changed(false) {}
};

/** Build the editable form. Returns false (and sets ir.failure) if the function must be left alone. */
bool decode_function(sharp_function *fn, function_ir &ir);

/**
 * Produce the final words for the function without touching code_info.
 * forceReencode = true ignores the saved raw words and encodes every
 * instruction from its operands (used to prove decode/encode are exact).
 * Returns false (and sets `error`) if something cannot be encoded.
 */
bool encode_words(function_ir &ir, bool forceReencode, std::vector<opcode_instr> &out, std::string &error);

/** Write the function back into its code_info: code words, line table, try/catch table. */
bool encode_function(function_ir &ir, std::string &error);

// ---- editing helpers for passes ---------------------------------------------------

/** Position of an instruction id in ir.code, or -1 */
int index_of(function_ir &ir, int id);

/** Remove the instruction at `index`. Everything that pointed at it now points at the next instruction. */
void remove_instruction(function_ir &ir, size_t index);

/** Follow removals: the live instruction id (or IR_END) a reference currently means */
int resolve_reference(function_ir &ir, int id);

/** A new instruction with a fresh id (not yet inserted). */
ir_instruction make_instruction(function_ir &ir, uint8_t op, int32_t a = 0, int32_t b = 0, int32_t c = 0);

/** True if any branch, table entry or code-address constant refers to this instruction (it is a "label") */
bool is_referenced(function_ir &ir, int id);

/** Text form of one instruction, for reports and debugging */
std::string to_string(function_ir &ir, const ir_instruction &instr);

#endif //SHARP_OPT_FUNCTION_IR_H
