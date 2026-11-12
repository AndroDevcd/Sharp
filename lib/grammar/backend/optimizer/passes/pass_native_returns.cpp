//
// Pass: native returns   (-O, works across functions)
// ===================================================
// A function that returns a number object only so that the caller can take the
// number straight back out gets a second, hidden version that returns the
// number itself. No object is created for that call any more.
//
//     def scale(x: var, f: var): double { return x * f + 0.5; }
//     ...
//     total += scale(i, 0.5).value;
//
//     caller before                         caller after
//     ----------------------------------    ----------------------------------
//       call  scale                           call  scale$value
//       popobj2            ; the object       loadval egx        ; the number
//       getf  egx, slot 0  ; .value
//
//     scale                                 scale$value (new, hidden)
//     ----------------------------------    ----------------------------------
//       new_class std#double                  ...x * f + 0.5 -> bmr
//       dup                                   return_val bmr
//       ...x * f + 0.5 -> bmr                 ret
//       rstore bmr
//       call  std#double.double
//       popobj2
//       returnobj
//       ret
//
// WHO GETS THE NUMBER, WHO GETS THE OBJECT
//   Decided call by call. A call is redirected only if the very next thing the
//   caller does with the result is read one number field out of it and drop
//   the object:
//
//         call F ; popobj2 ; getf r, slot k
//
//   Every other caller - one that stores the object, passes it on, compares
//   it, calls a method on it - still calls the original function and still
//   gets a real object. The original function is never changed.
//
// HOW THE SECOND VERSION IS MADE (and why it cannot be wrong)
//   It starts as an exact copy of the function in which every `returnobj` is
//   replaced by "read field k of that object and return the number":
//
//         returnobj        ->        getf ebx, slot k ; return_val ebx
//
//   That copy returns F(...).field[k] by construction - it still creates the
//   object. Then the ordinary box elimination pass runs on it. Now that the
//   object is no longer returned, it usually does not escape any more, and the
//   pass removes it exactly as it removes any other local box, with all of its
//   safety rules. If box elimination removes nothing, the copy would be no
//   faster than the original, so it is thrown away and the call stays as it was.
//
//   A function that returns null: the copy raises the null pointer exception
//   at its `return`, where the caller would have raised it at `.value`.
//
// WHAT IS NOT TOUCHED
//   Calls through interfaces, delegates and function pointers (the target is
//   not known), functions with try/catch/finally, native functions.
//

#include "passes.h"
#include "../analysis/control_flow.h"
#include "../../types/types.h"
#include "../../finalizer/generation/generator.h"
#include <memory>

extern uInt functionAddressCounter;     // finalizer/generation/function_generator.cpp

/* a function longer than this is not copied */
#define MAX_COPIED_INSTRUCTIONS 400
/* ...and no more than this many copies per program */
#define MAX_COPIES 400

namespace {

int count_allocations(function_ir &ir) {
    int count = 0;
    for(ir_instruction &instr : ir.code) {
        if(instr.op == Opcode::NEWCLASS) count++;
    }
    return count;
}

bool can_copy(function_ir &callee) {
    if(!callee.failure.empty() || callee.code.empty() || !callee.tries.empty()) return false;
    if(callee.code.size() > MAX_COPIED_INSTRUCTIONS) return false;

    function_type type = callee.fn->type;
    if(type == delegate_function || type == native_function || type == blueprint_function
       || type == undefined_function)
        return false;

    // returns exactly one value, and that value is an object
    if(callee.ci->spOffset != callee.ci->fpOffset) return false;
    if(is_numeric_type(callee.fn->returnType) && !callee.fn->returnType.isArray) return false;
    if(callee.fn->returnType.isArray) return false;

    // every way out must go through `returnobj ; ret`
    bool returns = false;
    for(size_t i = 0; i < callee.code.size(); i++) {
        const ir_instruction &instr = callee.code[i];
        const instruction_info &info = instruction_info_of(instr.op);

        if(instr.pcConstant || (info.flags & (f_indirect_jump | f_reads_pc))) return false;
        if(instr.op == Opcode::RETURNVAL || instr.op == Opcode::HLT) return false;
        if(instr.op == Opcode::RET) {
            if(instr.a != 0 || i == 0 || callee.code[i - 1].op != Opcode::RETURNOBJ) return false;
            if(is_referenced(callee, instr.id)) return false;     // something jumps to the `ret`, past the returnobj
            returns = true;
        }
    }
    return returns;
}

/**
 * The trial copy: `callee` returning field `field` of its result as a number.
 * Still attached to the original's sharp_function / code_info (nothing of the
 * original is written to); returns null if it would not save the allocation.
 */
std::unique_ptr<function_ir> make_trial(function_ir &callee, int field, program_ir &program,
                                        void (*cleanup)(function_ir&)) {
    std::unique_ptr<function_ir> copy(new function_ir(callee));

    for(size_t i = 0; i < copy->code.size(); i++) {
        if(copy->code[i].op != Opcode::RETURNOBJ) continue;

        // fp->obj = *ptr     ->     ebx = ptr->o.<number slot k> ; fp->var = ebx
        ir_instruction &instr = copy->code[i];
        instr.op = Opcode::GETF;
        instr.a = EBX; instr.b = field; instr.c = 0;
        instr.modified = true;

        ir_instruction give = make_instruction(*copy, Opcode::RETURNVAL, EBX);
        copy->code.insert(copy->code.begin() + i + 1, give);
        i += 1;
    }

    int before = count_allocations(*copy);
    cleanup(*copy);
    pass_box_elimination(*copy, program);
    if(count_allocations(*copy) >= before) return nullptr;      // the object is still created: no gain

    cleanup(*copy);
    return copy;
}

/** Give the trial copy a function of its own, so it is written to the executable. */
void materialize(function_ir &copy, function_ir &original, int field) {
    sharp_function *fn = new sharp_function(*original.fn);
    fn->name += "$value";
    fn->fullName += "$value";
    if(field != 0) {
        fn->name += std::to_string(field);
        fn->fullName += std::to_string(field);
    }
    fn->used = true;

    // it returns a plain number now
    sharp_type number;
    number.type = type_var;
    fn->returnType.copy(number);

    fn->ci = new code_info(*original.fn->ci);       // same frame layout and line table
    fn->ci->address = (int)functionAddressCounter++;
    fn->ci->uuid = UUIDGenerator++;

    copy.fn = fn;
    copy.ci = fn->ci;
    copy.changed = true;                            // the driver encodes it into fn->ci

    compressedCompilationFunctions.add(fn);
}

/** Does anything read through ptr after `index` before ptr is set again (within the block)? */
bool pointer_read_later(function_ir &ir, control_flow &cfg, int index) {
    int last = cfg.blocks[cfg.blockOf[index]].last;
    for(int i = index + 1; i <= last; i++) {
        const instruction_info &info = instruction_info_of(ir.code[i].op);
        if((info.flags & f_reads_ptr) || ir.code[i].op == Opcode::POPOBJ || ir.code[i].op == Opcode::DEL) return true;
        if(info.flags & (f_writes_ptr | f_call | f_opaque)) return false;
    }
    return false;
}

} // namespace

Int pass_native_returns(program_ir &program, std::vector<std::unique_ptr<function_ir>> &storage,
                        void (*cleanup)(function_ir&)) {
    // (callee address, field) -> address of the native version, or -1 = tried, not worth it
    std::map<std::pair<Int, int>, Int> versions;
    Int redirected = 0, created = 0;

    // program.functions grows while we go: the new versions are scanned too,
    // which is what turns a recursive function into a fully native recursion
    for(size_t f = 0; f < program.functions.size(); f++) {
        function_ir &caller = *program.functions[f];
        if(!caller.failure.empty()) continue;

        bool again = true, touched = false;
        while(again) {
            again = false;

            control_flow cfg;
            build_control_flow(caller, cfg);

            for(int i = 0; i + 2 < (int)caller.code.size(); i++) {
                // call F ; popobj2 ; getf r, slot k      - all in one block
                if(caller.code[i].op != Opcode::CALL || caller.code[i + 1].op != Opcode::POPOBJ_2
                   || caller.code[i + 2].op != Opcode::GETF)
                    continue;
                if(cfg.blockOf[i] != cfg.blockOf[i + 2]) continue;
                if(pointer_read_later(caller, cfg, i + 2)) continue;

                Int address = caller.code[i].a;
                int field = caller.code[i + 2].b;

                auto key = std::make_pair(address, field);
                auto known = versions.find(key);

                if(known == versions.end()) {
                    Int result = -1;
                    auto found = program.byAddress.find(address);

                    if(found != program.byAddress.end() && created < MAX_COPIES && can_copy(*found->second)) {
                        function_ir &original = *found->second;
                        std::unique_ptr<function_ir> trial = make_trial(original, field, program, cleanup);

                        if(trial) {
                            materialize(*trial, original, field);
                            result = trial->ci->address;

                            program.functions.push_back(trial.get());
                            program.byAddress[result] = trial.get();
                            storage.push_back(std::move(trial));
                            created++;
                        }
                    }

                    versions[key] = result;
                    known = versions.find(key);
                }

                if(known->second < 0) continue;

                // call F$value ; loadval r
                int reg = caller.code[i + 2].a;
                caller.code[i].a = (int32_t)known->second;
                caller.code[i].modified = true;

                ir_instruction &take = caller.code[i + 1];
                take.op = Opcode::LOADVAL;
                take.a = reg; take.b = take.c = 0;
                take.modified = true;

                remove_instruction(caller, (size_t)i + 2);

                caller.changed = true;
                redirected++;
                touched = true;
                again = true;       // indexes moved: rebuild the blocks and continue
                break;
            }
        }

        if(touched) cleanup(caller);    // the number now arrives through a push/pop pair worth forwarding
    }

    program.nativeReturnVersions += created;
    return redirected;
}
