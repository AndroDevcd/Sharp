//
// Sharp bytecode optimizer - what does a call do to a freshly created object?
// See box_constructors.h.
//
// The executor below is a tiny interpreter that runs a function on SYMBOLS
// instead of values:
//
//     objects   "self" (the object under construction) or "something else"
//     numbers   "the argument, after these casts", "the constant c", or "unknown"
//     ptr       "a local slot", "self's field 0", "a class's static object",
//               "the static-init flag of a class", or "something else"
//
// It follows calls into the callee (the callee's frame is built from the pushed
// values exactly as the VM does it) and follows a branch only when the
// condition is a known constant. Whatever it has no rule for makes the whole
// summary `box_unknown`, so the list of instructions below is also the complete
// list of things a "plain box constructor" may contain.
//

#include "box_constructors.h"
#include "../passes/passes.h"
#include "../../types/types.h"

#define MAX_DEPTH 8          /* nested calls followed */
#define MAX_STEPS 600        /* instructions executed in total */

namespace {

enum number_kind : uint8_t { n_unknown, n_constant, n_argument };

struct number_value {
    number_kind kind = n_unknown;
    long long constant = 0;
    std::vector<uint8_t> casts;     // for n_argument
};

/** One stack / frame slot: like the VM's, it has a number part and an object part */
struct slot_value {
    number_value number;
    bool isSelf = false;            // object part: the object under construction?
};

enum pointer_kind : uint8_t { p_other, p_local, p_self_value, p_static, p_static_flag };

struct executor {
    program_ir &program;
    std::vector<slot_value> stack;  // the operand stack, shared by all frames like the real one
    int steps = 0;
    int stores = 0;                 // writes to self.field[0]
    std::vector<uint8_t> casts;     // casts of the stored value

    explicit executor(program_ir &p) : program(p) {}

    /** Run `fn`; its frame is the top `arguments` stack entries. False = not understood. */
    bool run(function_ir &fn, int depth);
};

bool executor::run(function_ir &fn, int depth) {
    if(depth > MAX_DEPTH || !fn.failure.empty() || fn.code.empty()) return false;
    if(!fn.tries.empty()) return false;

    // the frame: slot 0..fpOffset are the pushed values, the rest are locals
    int arguments = fn.ci->fpOffset + 1;
    bool returnsValue = fn.ci->spOffset == fn.ci->fpOffset;
    if(returnsValue || fn.ci->spOffset != fn.ci->fpOffset + 1) return false; // constructors return nothing
    if(arguments < 0 || arguments > (int)stack.size()) return false;

    int slots = (fn.frameSlots > arguments ? fn.frameSlots : arguments) + 1;
    std::vector<slot_value> frame(slots);
    for(int i = arguments - 1; i >= 0; i--) {
        frame[i] = stack.back();
        stack.pop_back();
    }
    size_t stackFloor = stack.size();   // this function may not pop below its own pushes

    number_value registers[10];
    pointer_kind ptr = p_other;
    int ptrSlot = 0;

    auto local = [&](int32_t n) -> slot_value* { return n >= 0 && n < slots ? &frame[n] : nullptr; };
    auto reg = [&](int32_t r) -> number_value* { return r >= 0 && r < 10 ? &registers[r] : nullptr; };

    size_t pc = 0;
    while(pc < fn.code.size()) {
        if(++steps > MAX_STEPS) return false;

        const ir_instruction &instr = fn.code[pc];
        size_t next = pc + 1;

        switch(instr.op) {
            case Opcode::NOP:
                break;

            // ---- numbers
            case Opcode::MOVI: {
                number_value *r = reg(instr.a); if(!r || instr.pcConstant) return false;
                r->kind = n_constant; r->constant = instr.b; r->casts.clear();
                break;
            }
            case Opcode::MOVR: {
                number_value *to = reg(instr.a), *from = reg(instr.b); if(!to || !from) return false;
                *to = *from;
                break;
            }
            case Opcode::LOADL: {
                number_value *r = reg(instr.a); slot_value *s = local(instr.b); if(!r || !s) return false;
                *r = s->number;
                break;
            }
            case Opcode::SMOVR_2: {
                number_value *r = reg(instr.a); slot_value *s = local(instr.b); if(!r || !s) return false;
                s->number = *r;
                break;
            }
            case Opcode::SMOVR_4: {
                slot_value *to = local(instr.a), *from = local(instr.b); if(!to || !from) return false;
                to->number = from->number;
                break;
            }
            case Opcode::ISTOREL: {
                slot_value *s = local(instr.a); if(!s) return false;
                s->number.kind = n_constant; s->number.constant = instr.b; s->number.casts.clear();
                break;
            }
            case Opcode::ISTORE: {
                slot_value pushed;
                pushed.number.kind = n_constant; pushed.number.constant = instr.a;
                stack.push_back(pushed);
                break;
            }
            case Opcode::IPUSHL: {
                slot_value *s = local(instr.a); if(!s) return false;
                slot_value pushed; pushed.number = s->number;
                stack.push_back(pushed);
                break;
            }
            case Opcode::RSTORE: {
                number_value *r = reg(instr.a); if(!r) return false;
                slot_value pushed; pushed.number = *r;
                stack.push_back(pushed);
                break;
            }
            case Opcode::LOADVAL: {
                number_value *r = reg(instr.a); if(!r || stack.size() <= stackFloor) return false;
                *r = stack.back().number;
                stack.pop_back();
                break;
            }
            case Opcode::IPOPL: {
                slot_value *s = local(instr.a); if(!s || stack.size() <= stackFloor) return false;
                s->number = stack.back().number;
                stack.pop_back();
                break;
            }
            case Opcode::MOV8: case Opcode::MOV16: case Opcode::MOV32: case Opcode::MOV64:
            case Opcode::MOVU8: case Opcode::MOVU16: case Opcode::MOVU32: case Opcode::MOVU64: {
                number_value *to = reg(instr.a), *from = reg(instr.b); if(!to || !from) return false;
                number_value result = *from;
                if(result.kind == n_argument) result.casts.push_back(instr.op);
                else result.kind = n_unknown;
                *to = result;
                break;
            }

            // ---- objects
            case Opcode::MOVL:                              // ptr = &(fp+a)->obj
                if(!local(instr.a)) return false;
                ptr = p_local; ptrSlot = instr.a;
                break;
            case Opcode::PUSHOBJ: {                         // push *ptr
                if(ptr != p_local) return false;
                slot_value pushed; pushed.isSelf = frame[ptrSlot].isSelf;
                stack.push_back(pushed);
                break;
            }
            case Opcode::PUSHL: {
                slot_value *s = local(instr.a); if(!s) return false;
                slot_value pushed; pushed.isSelf = s->isSelf;
                stack.push_back(pushed);
                break;
            }
            case Opcode::POPL: {
                slot_value *s = local(instr.a); if(!s || stack.size() <= stackFloor) return false;
                s->isSelf = stack.back().isSelf;
                stack.pop_back();
                break;
            }
            case Opcode::DUP: {
                if(stack.size() <= stackFloor) return false;
                slot_value pushed; pushed.isSelf = stack.back().isSelf;
                stack.push_back(pushed);
                break;
            }
            case Opcode::POP:
                if(stack.size() <= stackFloor) return false;
                stack.pop_back();
                break;

            // ---- fields
            case Opcode::MOVG:                              // ptr = a class's static object
                ptr = p_static;
                break;
            case Opcode::MOVN:                              // ptr = &ptr->field[a]
                if(instr.a != 0) return false;
                if(ptr == p_local && frame[ptrSlot].isSelf) ptr = p_self_value;
                else if(ptr == p_static) ptr = p_static_flag;
                else return false;
                break;
            case Opcode::ILOAD: {                           // regs[a] = *ptr
                number_value *r = reg(instr.a);
                if(!r || ptr != p_static_flag) return false;
                r->kind = n_constant; r->constant = 1; r->casts.clear();   // see ONE ASSUMPTION in the header
                break;
            }
            case Opcode::IMOV: {                            // *ptr = regs[a]
                number_value *r = reg(instr.a); if(!r) return false;
                if(ptr == p_static_flag) {
                    if(r->kind != n_constant || r->constant != 1) return false;   // "initialized = true"
                } else if(ptr == p_self_value) {
                    if(r->kind != n_argument || stores != 0) return false;
                    stores++;
                    casts = r->casts;
                } else return false;
                break;
            }

            // ---- control flow
            case Opcode::JMP: case Opcode::JE: case Opcode::JNE: {
                bool taken = true;
                if(instr.op != Opcode::JMP) {
                    number_value &condition = registers[CMT];
                    if(condition.kind != n_constant) return false;
                    taken = instr.op == Opcode::JE ? condition.constant != 0 : condition.constant == 0;
                }
                if(taken) {
                    int target = resolve_reference(fn, instr.target);
                    if(target == IR_END) return true;
                    int index = target >= 0 ? index_of(fn, target) : -1;
                    if(index < 0) return false;
                    next = (size_t)index;
                }
                break;
            }
            case Opcode::CALL: {
                auto found = program.byAddress.find(instr.a);
                if(found == program.byAddress.end() || found->second == &fn) return false;
                if(found->second->ci->fpOffset + 1 > (int)(stack.size() - stackFloor)) return false;
                if(!run(*found->second, depth + 1)) return false;
                ptr = p_other;
                for(number_value &r : registers) r = number_value();
                break;
            }
            case Opcode::RET:
                if(instr.a != 0) return false;
                return stack.size() == stackFloor;

            default:
                if(getenv("SHARP_BOX_DEBUG")) fprintf(stderr, "box: %s stops at %s\n", fn.fn->fullName.c_str(), to_string(fn, instr).c_str());
                return false;
        }

        pc = next;
    }

    return stack.size() == stackFloor; // ran off the end: same as ret
}

} // namespace

const box_summary& summarize_box_call(program_ir &program, Int address) {
    auto cached = program.boxSummaries.find(address);
    if(cached != program.boxSummaries.end()) return cached->second;

    box_summary &summary = program.boxSummaries[address]; // box_unknown until proven otherwise

    auto found = program.byAddress.find(address);
    if(found == program.byAddress.end()) return summary;

    function_ir &fn = *found->second;
    if(!fn.failure.empty()) return summary;

    // shape: an instance function taking nothing, or one plain number
    int arguments = fn.ci->fpOffset + 1;
    int parameters = (int)fn.fn->parameters.size();
    if(arguments - parameters != 1 || parameters > 1) return summary;
    if(parameters == 1) {
        sharp_type &type = fn.fn->parameters.get(0)->type;
        if(!is_numeric_type(type) || type.isArray) return summary;
    }

    executor exec(program);
    slot_value self; self.isSelf = true;
    exec.stack.push_back(self);
    if(parameters == 1) {
        slot_value argument; argument.number.kind = n_argument;
        exec.stack.push_back(argument);
    }

    if(!exec.run(fn, 0)) return summary;

    if(exec.stores == 0 && parameters == 0) {
        summary.effect = box_no_effect;
    } else if(exec.stores == 1 && parameters == 1) {
        summary.effect = box_stores_value;
        summary.casts = exec.casts;
    }

    return summary;
}
