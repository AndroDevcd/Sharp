//
// Sharp bytecode optimizer - decode / encode of the editable function form
// See function_ir.h.
//

#include "function_ir.h"
#include "../../../core/opcode/opcode_macros.h"
#include <sstream>

// largest operand each layout can hold (see the bit layouts in opcode_macros.h)
#define MAX_D_OPERAND  0x7FFFFF   // 23 bits
#define MAX_CA_OPERAND 0xF        // 4 bits
#define MAX_CB_OPERAND 0x3FFFF    // 18 bits
#define MAX_B_OPERAND  0x7F       // 7 bits

// ---------------------------------------------------------------------------------
// decode
// ---------------------------------------------------------------------------------

static void decode_operands(ir_instruction &instr, const instruction_info &info, opcode_instr w0, opcode_instr w1) {
    instr.a = instr.b = instr.c = 0;

    switch(info.layout) {
        case layout_none:
            break;
        case layout_d:
            instr.a = GET_Da(w0);
            break;
        case layout_c:
            instr.a = GET_Ca(w0);
            instr.b = GET_Cb(w0);
            break;
        case layout_b:
            instr.a = GET_Ba(w0);
            instr.b = GET_Bb(w0);
            instr.c = GET_Bc(w0);
            break;
        case layout_raw:
            instr.a = (int32_t)w1;
            break;
        case layout_d_raw:
            instr.a = GET_Da(w0);
            instr.b = (int32_t)w1;
            break;
        case layout_d_c:
            instr.a = GET_Da(w0);
            instr.b = GET_Ca(w1);
            instr.c = GET_Cb(w1);
            break;
    }
}

/** instruction id that starts at `pc`, IR_END for "end of function", IR_NONE for -1, -3 if pc is not a valid place */
static int id_at(const std::vector<int> &pcToId, Int pc) {
    if(pc == -1) return IR_NONE;
    if(pc == (Int)pcToId.size()) return IR_END;
    if(pc < 0 || pc > (Int)pcToId.size()) return -3;
    return pcToId[pc] >= 0 ? pcToId[pc] : -3;
}

bool decode_function(sharp_function *fn, function_ir &ir) {
    ir = function_ir();
    ir.fn = fn;
    ir.ci = fn->ci;
    code_info *ci = fn->ci;
    ir.frameSlots = ci->stackSize > 0 ? ci->stackSize : 0;

    if(ci->hasAsm) {
        ir.failure = "contains hand written asm";
        return false;
    }

    Int size = ci->code.size();
    std::vector<int> pcToId(size, -1);
    std::vector<Int> pcOfIndex;

    // ---- pass 1: split the words into instructions
    for(Int pc = 0; pc < size; ) {
        opcode_instr w0 = ci->code.get(pc);
        const instruction_info &info = instruction_info_of(GET_OP(w0));

        if(!known(info)) {
            std::stringstream ss;
            ss << "unknown opcode 0x" << std::hex << (int)GET_OP(w0) << std::dec << " at pc " << pc;
            ir.failure = ss.str();
            return false;
        }

        int words = words_of(info);
        if(pc + words > size) {
            ir.failure = "instruction runs past the end of the function";
            return false;
        }

        if(info.flags & f_reads_pc) {
            ir.failure = "reads its own code address (loadpc)";
            return false;
        }

        ir_instruction instr;
        instr.id = ir.nextId++;
        instr.op = GET_OP(w0);
        instr.raw[0] = w0;
        instr.raw[1] = words == 2 ? ci->code.get(pc + 1) : 0;
        instr.modified = false;
        instr.target = IR_NONE;
        instr.pcConstant = false;
        decode_operands(instr, info, instr.raw[0], instr.raw[1]);

        pcToId[pc] = instr.id;
        pcOfIndex.push_back(pc);
        ir.code.push_back(instr);
        pc += words;
    }
    ir.forward.assign(ir.nextId, IR_NONE);

    // ---- pass 2: turn code addresses into instruction ids
    for(size_t i = 0; i < ir.code.size(); i++) {
        ir_instruction &instr = ir.code[i];
        const instruction_info &info = instruction_info_of(instr.op);
        Int pc = pcOfIndex[i];
        Int targetPc = -1;
        bool hasTarget = true;

        if(info.a == k_pc) targetPc = instr.a;
        else if(info.a == k_pc_rel) targetPc = pc + instr.a + 1;   // SKIP: pc += a, then the usual pc++
        else if(info.b == k_pc_rel) targetPc = pc + instr.b;       // SKPE / SKNE: pc += b
        else hasTarget = false;

        if(hasTarget) {
            instr.target = id_at(pcToId, targetPc);
            if(instr.target == -3 || instr.target == IR_NONE) {
                std::stringstream ss;
                ss << "branch at pc " << pc << " to " << targetPc << " which is not the start of an instruction";
                ir.failure = ss.str();
                return false;
            }
        }
    }

    // code addresses stored as plain numbers, tagged by the generator
    for(Int i = 0; i < (Int)ci->labelOperands.size(); i++) {
        Int pc = ci->labelOperands.get(i);
        int id = id_at(pcToId, pc);
        ir_instruction *instr = id >= 0 ? &ir.code[index_of(ir, id)] : nullptr;

        if(instr == nullptr || instr->op != Opcode::MOVI) {
            ir.failure = "a code address is used as an operand of something other than movi";
            return false;
        }

        instr->target = id_at(pcToId, instr->b);
        if(instr->target == -3 || instr->target == IR_NONE) {
            ir.failure = "movi holds a code address that is not the start of an instruction";
            return false;
        }
        instr->pcConstant = true;
    }

    // ---- line table
    for(Int i = 0; i < (Int)ci->lineTable.size(); i++) {
        ir_line_entry entry;
        entry.at = id_at(pcToId, ci->lineTable.get(i)->start_pc);
        entry.line = ci->lineTable.get(i)->line;

        if(entry.at == -3 || entry.at == IR_NONE) {
            ir.failure = "line table entry is not at the start of an instruction";
            return false;
        }
        ir.lines.push_back(entry);
    }

    // ---- try / catch / finally table
    for(Int i = 0; i < (Int)ci->tryCatchTable.size(); i++) {
        try_catch_data *data = ci->tryCatchTable.get(i);
        ir_try_entry entry;
        entry.tryStart = id_at(pcToId, data->tryStartPc);
        entry.tryEnd = id_at(pcToId, data->tryEndPc);
        entry.blockStart = id_at(pcToId, data->blockStartPc);
        entry.blockEnd = id_at(pcToId, data->blockEndPc);
        entry.hasFinally = data->finallyData != NULL;
        entry.finallyStart = entry.finallyEnd = IR_NONE;

        bool valid = entry.tryStart != -3 && entry.tryEnd != -3 && entry.blockStart != -3 && entry.blockEnd != -3;

        for(Int j = 0; j < (Int)data->catchTable.size(); j++) {
            int handler = id_at(pcToId, data->catchTable.get(j)->handlerPc);
            if(handler == -3) valid = false;
            entry.handlers.push_back(handler);
        }

        if(entry.hasFinally) {
            entry.finallyStart = id_at(pcToId, (Int)data->finallyData->startPc);
            entry.finallyEnd = id_at(pcToId, (Int)data->finallyData->endPc);
            if(entry.finallyStart == -3 || entry.finallyEnd == -3) valid = false;
        }

        if(!valid) {
            ir.failure = "try/catch table entry is not at the start of an instruction";
            return false;
        }
        ir.tries.push_back(entry);
    }

    return true;
}

// ---------------------------------------------------------------------------------
// editing helpers
// ---------------------------------------------------------------------------------

int index_of(function_ir &ir, int id) {
    // ids are handed out in order, so an untouched function has id == index
    if(id >= 0 && id < (int)ir.code.size() && ir.code[id].id == id)
        return id;

    for(size_t i = 0; i < ir.code.size(); i++) {
        if(ir.code[i].id == id) return (int)i;
    }
    return -1;
}

int resolve_reference(function_ir &ir, int id) {
    while(id >= 0 && ir.forward[id] != IR_NONE)
        id = ir.forward[id];
    return id;
}

void remove_instruction(function_ir &ir, size_t index) {
    int id = ir.code[index].id;
    ir.forward[id] = index + 1 < ir.code.size() ? ir.code[index + 1].id : IR_END;
    ir.code.erase(ir.code.begin() + index);
}

ir_instruction make_instruction(function_ir &ir, uint8_t op, int32_t a, int32_t b, int32_t c) {
    ir_instruction instr;
    instr.id = ir.nextId++;
    instr.op = op;
    instr.a = a; instr.b = b; instr.c = c;
    instr.raw[0] = instr.raw[1] = 0;
    instr.modified = true;
    instr.target = IR_NONE;
    instr.pcConstant = false;
    ir.forward.push_back(IR_NONE);
    return instr;
}

bool is_referenced(function_ir &ir, int id) {
    for(ir_instruction &instr : ir.code) {
        if(instr.target != IR_NONE && resolve_reference(ir, instr.target) == id) return true;
    }
    for(ir_line_entry &line : ir.lines) {
        // a line table entry is not a jump target: it does not make an instruction a label
        (void)line;
    }
    for(ir_try_entry &entry : ir.tries) {
        if(resolve_reference(ir, entry.tryStart) == id || resolve_reference(ir, entry.tryEnd) == id
           || resolve_reference(ir, entry.blockStart) == id || resolve_reference(ir, entry.blockEnd) == id
           || resolve_reference(ir, entry.finallyStart) == id || resolve_reference(ir, entry.finallyEnd) == id)
            return true;

        for(int handler : entry.handlers) {
            if(resolve_reference(ir, handler) == id) return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------------
// encode
// ---------------------------------------------------------------------------------

static bool fits(int32_t value, int32_t max) {
    return value >= -max && value <= max;
}

static uint32_t magnitude(int32_t value) { return (uint32_t)(value < 0 ? -(int64_t)value : value); }
static uint32_t sign_bit(int32_t value)  { return value < 0 ? NEGATIVE : POSITIVE; }

static bool encode_instruction(const ir_instruction &instr, opcode_instr out[2], std::string &error) {
    const instruction_info &info = instruction_info_of(instr.op);
    opcode_instr tmp = 0;
    bool ok = true;
    out[0] = out[1] = 0;

    switch(info.layout) {
        case layout_none:
            out[0] = SET_Ei(tmp, instr.op);
            break;
        case layout_d:
            ok = fits(instr.a, MAX_D_OPERAND);
            out[0] = SET_Di(tmp, instr.op, magnitude(instr.a), sign_bit(instr.a));
            break;
        case layout_c:
            ok = fits(instr.a, MAX_CA_OPERAND) && fits(instr.b, MAX_CB_OPERAND);
            out[0] = SET_Ci(tmp, instr.op, magnitude(instr.a), sign_bit(instr.a), magnitude(instr.b), sign_bit(instr.b));
            break;
        case layout_b:
            ok = fits(instr.a, MAX_B_OPERAND) && fits(instr.b, MAX_B_OPERAND) && fits(instr.c, MAX_B_OPERAND);
            out[0] = SET_Bi(tmp, instr.op, magnitude(instr.a), sign_bit(instr.a), magnitude(instr.b), sign_bit(instr.b),
                            magnitude(instr.c), sign_bit(instr.c));
            break;
        case layout_raw:
            out[0] = SET_Ei(tmp, instr.op);
            out[1] = (opcode_instr)instr.a;
            break;
        case layout_d_raw:
            ok = fits(instr.a, MAX_D_OPERAND);
            out[0] = SET_Di(tmp, instr.op, magnitude(instr.a), sign_bit(instr.a));
            out[1] = (opcode_instr)instr.b;
            break;
        case layout_d_c:
            ok = fits(instr.a, MAX_D_OPERAND) && fits(instr.b, MAX_CA_OPERAND) && fits(instr.c, MAX_CB_OPERAND);
            out[0] = SET_Di(tmp, instr.op, magnitude(instr.a), sign_bit(instr.a));
            out[1] = SET_Ci(tmp, Opcode::ILL, magnitude(instr.b), sign_bit(instr.b), magnitude(instr.c), sign_bit(instr.c));
            break;
    }

    if(!ok) {
        std::stringstream ss;
        ss << "operand of `" << info.name << "` does not fit in the instruction";
        error = ss.str();
    }
    return ok;
}

/** new pc of every instruction id, and the total size in words */
static Int layout_code(function_ir &ir, std::vector<Int> &pcOf) {
    pcOf.assign(ir.nextId, -1);
    Int pc = 0;
    for(ir_instruction &instr : ir.code) {
        pcOf[instr.id] = pc;
        pc += words_of(instruction_info_of(instr.op));
    }
    return pc;
}

static Int pc_of_reference(function_ir &ir, const std::vector<Int> &pcOf, Int total, int reference) {
    if(reference == IR_NONE) return -1;
    int id = resolve_reference(ir, reference);
    return id == IR_END ? total : pcOf[id];
}

bool encode_words(function_ir &ir, bool forceReencode, std::vector<opcode_instr> &out, std::string &error) {
    std::vector<Int> pcOf;
    Int total = layout_code(ir, pcOf);
    out.clear();
    out.reserve(total);

    for(ir_instruction &instr : ir.code) {
        const instruction_info &info = instruction_info_of(instr.op);
        Int pc = pcOf[instr.id];

        // code addresses follow the instructions they point at
        if(instr.target != IR_NONE) {
            Int targetPc = pc_of_reference(ir, pcOf, total, instr.target);
            int32_t *operand;
            int32_t value;

            if(instr.pcConstant)            { operand = &instr.b; value = (int32_t)targetPc; }
            else if(info.a == k_pc)         { operand = &instr.a; value = (int32_t)targetPc; }
            else if(info.a == k_pc_rel)     { operand = &instr.a; value = (int32_t)(targetPc - pc - 1); }
            else                            { operand = &instr.b; value = (int32_t)(targetPc - pc); }

            if(*operand != value) {
                *operand = value;
                instr.modified = true;
            }
        }

        opcode_instr words[2];
        if(instr.modified || forceReencode) {
            if(!encode_instruction(instr, words, error)) return false;
        } else {
            words[0] = instr.raw[0];
            words[1] = instr.raw[1];
        }

        out.push_back(words[0]);
        if(words_of(info) == 2) out.push_back(words[1]);
    }

    return true;
}

bool encode_function(function_ir &ir, std::string &error) {
    std::vector<opcode_instr> words;
    if(!encode_words(ir, false, words, error))
        return false;

    std::vector<Int> pcOf;
    Int total = layout_code(ir, pcOf);
    code_info *ci = ir.ci;

    ci->code.free();
    for(opcode_instr word : words) ci->code.add(word);

    for(size_t i = 0; i < ir.lines.size(); i++)
        ci->lineTable.get(i)->start_pc = pc_of_reference(ir, pcOf, total, ir.lines[i].at);

    for(size_t i = 0; i < ir.tries.size(); i++) {
        try_catch_data *data = ci->tryCatchTable.get(i);
        ir_try_entry &entry = ir.tries[i];

        data->tryStartPc = pc_of_reference(ir, pcOf, total, entry.tryStart);
        data->tryEndPc = pc_of_reference(ir, pcOf, total, entry.tryEnd);
        data->blockStartPc = pc_of_reference(ir, pcOf, total, entry.blockStart);
        data->blockEndPc = pc_of_reference(ir, pcOf, total, entry.blockEnd);

        for(size_t j = 0; j < entry.handlers.size(); j++)
            data->catchTable.get(j)->handlerPc = pc_of_reference(ir, pcOf, total, entry.handlers[j]);

        if(entry.hasFinally) {
            data->finallyData->startPc = (uInt)pc_of_reference(ir, pcOf, total, entry.finallyStart);
            data->finallyData->endPc = (uInt)pc_of_reference(ir, pcOf, total, entry.finallyEnd);
        }
    }

    // A pass asked for more frame slots: grow the frame the same way the code
    // generator sizes it (code_context.cpp, flush_context / set_frame_stack_offset):
    //   stackSize        = every slot of the frame
    //   frameStackOffset = the slots that are not parameters (and not `self`),
    //                      which the VM reserves on top of the arguments
    if(ir.frameSlots > ci->stackSize) {
        bool isStatic = check_flag(ir.fn->flags, flag_static);
        Int ownSlots = ir.frameSlots - (isStatic ? 0 : 1) - (Int)ir.fn->parameters.size();

        ci->stackSize = ir.frameSlots;
        ci->frameStackOffset = ownSlots > 0 ? ownSlots : 0;
    }

    // the tagged code-address constants moved too
    ci->labelOperands.free();
    for(ir_instruction &instr : ir.code) {
        if(instr.pcConstant) ci->labelOperands.add(pcOf[instr.id]);
    }

    return true;
}

// ---------------------------------------------------------------------------------
// text
// ---------------------------------------------------------------------------------

static const char* register_names[] = { "adx", "cx", "cmt", "ebx", "ecx", "ecf", "edf", "ehf", "bmr", "egx" };

static void operand_to_string(std::stringstream &ss, operand_kind kind, int32_t value) {
    switch(kind) {
        case k_none: break;
        case k_reg_r: case k_reg_w: case k_reg_rw:
            if(value >= 0 && value < 10) ss << " " << register_names[value];
            else ss << " reg" << value;
            break;
        case k_var_r: case k_var_w: case k_var_rw: ss << " fp" << (value >= 0 ? "+" : "") << value << ".var"; break;
        case k_obj_r: case k_obj_w: case k_slot_addr: ss << " fp" << (value >= 0 ? "+" : "") << value << ".obj"; break;
        case k_sp_rel: ss << " sp" << (value >= 0 ? "+" : "") << value; break;
        case k_pc: ss << " @" << value; break;
        case k_pc_rel: ss << " @+" << value; break;
        case k_method: ss << " fn#" << value; break;
        case k_class: ss << " class#" << value; break;
        default: ss << " " << value; break;
    }
}

std::string to_string(function_ir &ir, const ir_instruction &instr) {
    const instruction_info &info = instruction_info_of(instr.op);
    std::stringstream ss;
    ss << (known(info) ? info.name : "?");
    operand_to_string(ss, info.a, instr.a);
    operand_to_string(ss, info.b, instr.b);
    operand_to_string(ss, info.c, instr.c);
    if(instr.target != IR_NONE) ss << "  -> #" << resolve_reference(ir, instr.target);
    return ss.str();
}
