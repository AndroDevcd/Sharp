//
// See constant_conversion.h.
//
// HOW IT KNOWS WHAT THE LAST INSTRUCTION WAS
// cc.lastInstruction is the index of the instruction add_instruction() appended
// last, or -1 when that is not safe to look at:
//      - a label was just placed here (set_label): code jumps to this point, so
//        the register may come from another path
//      - a jump whose target is filled in later was just reserved
//        (create_dynamic_instruction)
//      - a new function was started
// cc.previousInstruction is the one before it, kept for the common shape
//
//      movi #3, edf        ; the right operand, a constant
//      i2d  bmr, bmr       ; the left operand is converted first
//      (convert edf)       ; <- we are here: the constant is one instruction back
//
// Looking at the raw words alone would not do: the second word of a two-word
// instruction is a plain number and can look like any opcode.
//

#include "constant_conversion.h"
#include "code_context.h"
#include "scheme/step_processor.h"
#include "../generator.h"
#include "../../../../../core/opcode/opcode_macros.h"
#include <cmath>
#include <cstdint>

namespace {

/** a constant the last instruction loaded into a register */
struct loaded_constant {
    bool found = false;
    bool decimal = false;       // dmovi / ldc
    double value = 0;
    Int words = 0;              // how long that instruction is
};

loaded_constant last_loaded_constant(_register r) {
    loaded_constant constant;
    Int at = cc.lastInstruction;
    Int size = (Int)cc.instructions.size();
    if(at < 0 || at >= size) return constant;

    opcode_instr word = cc.instructions.get(at);
    switch(GET_OP(word)) {
        case Opcode::MOVI:          // regs[a] = <second word>, an integer
        case Opcode::DMOVI:         // regs[a] = (double)<second word>
            if(at + 2 != size || GET_Da(word) != r) return constant;
            constant.found = true;
            constant.decimal = GET_OP(word) == Opcode::DMOVI;
            constant.value = (double)(int32_t)cc.instructions.get(at + 1);
            constant.words = 2;
            return constant;
        case Opcode::LDC:           // regs[a] = decimal constant b of the executable
            if(at + 1 != size || GET_Ca(word) != r) return constant;
            constant.found = true;
            constant.decimal = true;
            constant.value = constantMap.get(GET_Cb(word));
            constant.words = 1;
            return constant;
        default:
            return constant;
    }
}

/**
 * Does the last instruction leave register `r` alone? Only the few instructions
 * that can stand between a constant and its conversion are known here; anything
 * else answers no.
 */
bool last_instruction_ignores(_register r) {
    Int at = cc.lastInstruction;
    Int size = (Int)cc.instructions.size();
    if(at < 0 || at >= size) return false;

    opcode_instr word = cc.instructions.get(at);
    switch(GET_OP(word)) {
        case Opcode::I2D: case Opcode::D2I:     // regs[a] = convert(regs[b])
            return at + 1 == size && GET_Ca(word) != r && GET_Cb(word) != r;
        case Opcode::MOVI: case Opcode::DMOVI:  // regs[a] = constant
            return at + 2 == size && GET_Da(word) != r;
        case Opcode::LDC:
            return at + 1 == size && GET_Ca(word) != r;
        default:
            return false;
    }
}

/**
 * The constant is one instruction back (see the top of the file): MOVI and
 * DMOVI are the same size, so one is turned into the other where it stands.
 */
bool fold_previous_instruction(_register r, numeric_conversion conversion) {
    Int at = cc.previousInstruction;
    if(at < 0 || at + 2 != cc.lastInstruction || !last_instruction_ignores(r)) return false;

    opcode_instr word = cc.instructions.get(at);
    int op = GET_OP(word);
    if((op != Opcode::MOVI && op != Opcode::DMOVI) || GET_Da(word) != r) return false;

    Int value = (Int)(int32_t)cc.instructions.get(at + 1);
    opcode_instr *replacement;
    if(conversion == convert_to_decimal && op == Opcode::MOVI)
        replacement = Opcode::Builder::dmovi((opcode_arg)value, r);
    else if(conversion == convert_to_integer && op == Opcode::DMOVI)
        replacement = Opcode::Builder::movi((opcode_arg)value, r);
    else if(conversion == convert_to_truth && op == Opcode::DMOVI)
        replacement = Opcode::Builder::movi(value != 0 ? 1 : 0, r);
    else
        return false;

    cc.instructions.get(at) = replacement[0];
    cc.instructions.get(at + 1) = replacement[1];
    return true;
}

/** a whole number that MOVI / DMOVI can carry in their second word */
bool fits_in_instruction(double value) {
    return value == std::floor(value) && value > (double)INT32_MIN && value < (double)INT32_MAX;
}

void replace_last_instruction(const loaded_constant &old, opcode_instr *replacement) {
    for(Int i = 0; i < old.words; i++)
        cc.instructions.pop_back();
    add_instruction(replacement);
}

} // namespace

bool fold_constant_conversion(_register r, numeric_conversion conversion) {
    if(conversion == convert_nothing) return false;

    loaded_constant constant = last_loaded_constant(r);
    if(!constant.found) return fold_previous_instruction(r, conversion);

    switch(conversion) {
        case convert_to_decimal:
            // only an integer constant is converted; MOVI always fits DMOVI
            if(constant.decimal) return false;
            replace_last_instruction(constant, Opcode::Builder::dmovi((opcode_arg)constant.value, r));
            return true;

        case convert_to_integer:
            // the fraction is dropped at run time (D2I); only fold the cases
            // where there is nothing to drop and the value fits
            if(!constant.decimal || !fits_in_instruction(constant.value)) return false;
            replace_last_instruction(constant, Opcode::Builder::movi((opcode_arg)constant.value, r));
            return true;

        case convert_to_truth:
            // 1 when the decimal is not 0.0 (a NaN is not 0.0 either)
            if(!constant.decimal) return false;
            replace_last_instruction(constant, Opcode::Builder::movi(constant.value != 0 ? 1 : 0, r));
            return true;

        default:
            return false;
    }
}
