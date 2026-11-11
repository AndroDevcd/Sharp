//
// Pass: inline calls   (-O3, works across functions)
// ==================================================
// Replaces a `call` of a small function with a copy of that function's body.
//
// WHY IT MATTERS
//   A call is by far the most expensive thing ordinary Sharp code does: the VM
//   records a frame, moves fp/sp, switches the code pointer, and undoes it all
//   on return. Getters, setters, operators and the constructor chains behind
//   every boxed number are a handful of instructions each, so the call costs
//   many times more than the work it wraps. Inlined, the call cost is gone and
//   the other passes can then optimize across what used to be a call boundary.
//
// HOW A CALL WORKS (lib/runtime/virtual_machine.cpp, prepare_method / return_method)
//   The caller pushes `self` (for instance functions) and the arguments, then
//   `call`. The callee's frame starts ON those pushed values:
//
//       slot 0            self, or the first parameter of a static function
//       slot 1..          the remaining parameters, then the callee's locals
//
//   A function that returns a value writes it to slot 0 (RETURNVAL / RETURNOBJ)
//   and after `ret` slot 0 is the top of the caller's stack; everything above
//   it is dropped. A function that returns nothing drops the arguments too.
//
// WHAT INLINING DOES
//   The callee's slots become extra locals of the caller (a fresh block at the
//   end of the caller's frame, `base`):
//
//       source:    p.x          where   def get_x() := x;
//
//       before                           after
//       ---------------------------      ----------------------------------
//         pushl  fp+1        ; p           pushl  fp+1          ; p
//         call   get_x                     popl   fp+base+0     ; self = pop
//         loadval ecx                      movl   base+0        ; body of get_x,
//                                          movn   #0            ; locals moved
//                                          iload  ebx           ; by +base
//                                          smovr_2 ebx, fp+base+0   ; (was returnval)
//                                          ipushl base+0        ; result on the stack
//                                          loadval ecx
//
//     1. pop the arguments off the stack into the callee's parameter slots
//     2. the callee's instructions, with every local slot n -> base + n,
//        `returnval r` -> store r into slot 0, `returnobj` -> store *ptr into
//        slot 0, `ret` -> jump to the end of the inlined body
//     3. push slot 0 if the function returns a value
//   The stack forwarding pass usually removes most of steps 1 and 3 again.
//
// WHAT IS NOT INLINED (the call simply stays a call)
//   - functions longer than MAX_INLINE_INSTRUCTIONS
//   - functions with try/catch/finally, hand written asm, or anything else the
//     optimizer refuses to decode
//   - recursive calls, native functions, delegates (`invoke_delegate`: the
//     target is only known at run time)
//
// VISIBLE DIFFERENCE
//   An inlined function has no frame of its own, so it does not appear in
//   exception stack traces; the trace shows the caller's line of the call.
//

#include "passes.h"
#include "../../types/types.h"

/* a callee with more instructions than this is not worth copying into every caller */
#define MAX_INLINE_INSTRUCTIONS 24
/* stop growing a caller beyond this many instructions / frame slots */
#define MAX_CALLER_INSTRUCTIONS 3000
#define MAX_CALLER_FRAME_SLOTS 200

static bool is_number_slot(sharp_type &type) {
    return is_numeric_type(type) && !type.isArray;
}

static int slots_of(function_ir &fn) {
    return fn.frameSlots > 0 ? fn.frameSlots : 1; // slot 0 always exists: it is the return slot
}

static bool can_inline(function_ir &callee, function_ir &caller) {
    if(&callee == &caller) return false;
    if(!callee.failure.empty() || callee.code.empty()) return false;
    if(callee.code.size() > MAX_INLINE_INSTRUCTIONS) return false;
    if(callee.ci->tryCatchTable.size() != 0) return false;

    function_type type = callee.fn->type;
    if(type == delegate_function || type == native_function || type == blueprint_function
       || type == undefined_function)
        return false;

    // must end in a plain `ret` so "the end of the body" is a single place
    ir_instruction &last = callee.code.back();
    if(last.op != Opcode::RET || last.a != 0) return false;

    // the frame offsets must describe "self (optional) + parameters", nothing else
    int selfSlots = (callee.ci->fpOffset + 1) - (int)callee.fn->parameters.size();
    if(selfSlots != 0 && selfSlots != 1) return false;
    if(callee.ci->spOffset != callee.ci->fpOffset && callee.ci->spOffset != callee.ci->fpOffset + 1) return false;

    int slots = slots_of(callee);
    for(ir_instruction &instr : callee.code) {
        const instruction_info &info = instruction_info_of(instr.op);

        if(instr.pcConstant) return false;
        if(info.flags & (f_indirect_jump | f_opaque | f_reads_pc)) return false;
        if(instr.op == Opcode::HLT) return false;
        if(instr.op == Opcode::RET && instr.a != 0) return false;              // returns with an exception
        if(instr.op == Opcode::CALL && instr.a == callee.ci->address) return false; // recursive
        if(info.a == k_pc_rel || info.b == k_pc_rel) return false;
        // sp-relative addressing could reach into the callee's own frame, whose
        // position relative to sp is different once it lives in the caller's frame
        if(info.a == k_sp_rel || info.b == k_sp_rel || info.c == k_sp_rel) return false;

        const operand_kind kinds[3] = { info.a, info.b, info.c };
        const int32_t values[3] = { instr.a, instr.b, instr.c };
        for(int k = 0; k < 3; k++) {
            if(is_local_kind(kinds[k]) && (values[k] < 0 || values[k] >= slots)) return false;
        }
    }

    return true;
}

/** Replace the call at caller.code[callIndex] with the body of `callee`. Returns how many instructions took its place. */
static size_t inline_at(function_ir &caller, size_t callIndex, function_ir &callee) {
    // The VM places the callee's frame with two numbers the generator computed:
    //     fp = sp - fpOffset      -> slots 0..fpOffset are the pushed values
    //     sp = sp - spOffset      on return: spOffset == fpOffset leaves slot 0 on
    //                             the stack (a result), one more drops it too
    int parameters = (int)callee.fn->parameters.size();
    int arguments = callee.ci->fpOffset + 1;
    int selfSlots = arguments - parameters;        // 1 for instance functions, 0 for static
    bool isStatic = selfSlots == 0;
    bool returnsValue = callee.ci->spOffset == callee.ci->fpOffset;
    bool returnsNumber = returnsValue && is_number_slot(callee.fn->returnType);

    int base = caller.frameSlots > 0 ? caller.frameSlots : 1;
    caller.frameSlots = base + slots_of(callee);

    std::vector<ir_instruction> body;

    // ---- 1. arguments: the last one pushed is on top, so pop them in reverse
    for(int slot = arguments - 1; slot >= 0; slot--) {
        bool number = false;
        if(isStatic || slot > 0)
            number = is_number_slot(callee.fn->parameters.get(slot - (isStatic ? 0 : 1))->type);

        body.push_back(make_instruction(caller, number ? Opcode::IPOPL : Opcode::POPL, base + slot));
    }

    // ---- 3. (created first so jumps in the body have something to point at)
    // The instruction every `ret` of the body jumps to.
    int endId;
    ir_instruction pushResult;
    if(returnsValue) {
        pushResult = make_instruction(caller, returnsNumber ? Opcode::IPUSHL : Opcode::PUSHL, base);
        endId = pushResult.id;
    } else {
        endId = callIndex + 1 < caller.code.size() ? caller.code[callIndex + 1].id : IR_END;
    }

    // ---- 2. the body
    std::vector<int> newIdOf(callee.nextId, IR_NONE);
    std::vector<bool> targetIsCalleeId;   // per body entry: its target still names a callee instruction
    targetIsCalleeId.assign(body.size(), false);

    for(size_t i = 0; i < callee.code.size(); i++) {
        const ir_instruction &original = callee.code[i];
        const instruction_info &info = instruction_info_of(original.op);
        bool isLast = i + 1 == callee.code.size();
        ir_instruction copy;

        if(original.op == Opcode::RET) {
            if(isLast) {
                // falls straight into the end; keep a mapping so jumps to this `ret` land there
                newIdOf[original.id] = endId;
                continue;
            }
            copy = make_instruction(caller, Opcode::JMP, 0);
            copy.target = endId;
            newIdOf[original.id] = copy.id;
            body.push_back(copy);
            targetIsCalleeId.push_back(false);
            continue;
        } else if(original.op == Opcode::RETURNVAL) {
            copy = make_instruction(caller, Opcode::SMOVR_2, original.a, base);     // slot 0 = regs[a]
        } else if(original.op == Opcode::RETURNOBJ) {
            copy = make_instruction(caller, Opcode::SMOVR_3, base);                 // slot 0 = *ptr
        } else {
            copy = make_instruction(caller, original.op, original.a, original.b, original.c);
            copy.target = original.target; // still a callee id, fixed below

            if(is_local_kind(info.a)) copy.a += base;
            if(is_local_kind(info.b)) copy.b += base;
            if(is_local_kind(info.c)) copy.c += base;
        }

        newIdOf[original.id] = copy.id;
        body.push_back(copy);
        targetIsCalleeId.push_back(copy.target != IR_NONE && original.op != Opcode::RETURNVAL
                                   && original.op != Opcode::RETURNOBJ);
    }

    // Jump targets inside the body: callee ids -> the copies. (Callee ids and
    // caller ids are separate number spaces, hence the explicit flags.)
    for(size_t i = 0; i < body.size(); i++) {
        if(!targetIsCalleeId[i]) continue;

        int target = resolve_reference(callee, body[i].target);
        body[i].target = target == IR_END ? endId : newIdOf[target];
    }

    if(returnsValue) body.push_back(pushResult);

    // ---- splice it in where the call was
    int callId = caller.code[callIndex].id;
    if(body.empty()) {
        remove_instruction(caller, callIndex);
    } else {
        // anything that pointed at the call now points at the first inlined instruction
        caller.forward[callId] = body.front().id;
        caller.code.erase(caller.code.begin() + callIndex);
        caller.code.insert(caller.code.begin() + callIndex, body.begin(), body.end());
    }

    caller.changed = true;
    return body.size();
}

bool pass_inline_calls(function_ir &caller, program_ir &program) {
    bool changed = false;

    for(size_t i = 0; i < caller.code.size(); i++) {
        if(caller.code[i].op != Opcode::CALL) continue;
        if(caller.code.size() > MAX_CALLER_INSTRUCTIONS) break;

        auto found = program.byAddress.find(caller.code[i].a);
        if(found == program.byAddress.end()) continue;

        function_ir &callee = *found->second;
        if(!can_inline(callee, caller)) continue;
        if(caller.frameSlots + slots_of(callee) > MAX_CALLER_FRAME_SLOTS) continue;

        // Do not re-scan what was just inserted in this round: calls inside it are
        // looked at in the next round, after the callee itself had its turn.
        size_t inserted = inline_at(caller, i, callee);
        i = i + inserted - 1; // (wraps around and back when both are 0)
        program.callsInlined++;
        changed = true;
    }

    return changed;
}
