//
// Sharp bytecode optimizer - driver
// See bytecode_optimizer.h.
//

#include <cstring>
#include <cstdlib>
#include "bytecode_optimizer.h"
#include "function_ir.h"
#include "passes/passes.h"
#include "../finalizer/generation/generator.h"
#include "../../settings/settings.h"
#include "../../../core/opcode/opcode_macros.h"
#include <map>
#include <memory>
#include <iostream>

// ---------------------------------------------------------------------------------
// The pass list. Passes run in this order, repeatedly, until none of them
// changes the function anymore (one pass often exposes work for another).
// ---------------------------------------------------------------------------------
struct optimizer_pass {
    const char *name;
    int level;                          // lowest -O level it runs at
    bool (*run)(function_ir &ir);       // returns true if it changed the function
    Int instructionsRemoved;            // statistics for -Oreport
    Int timesChanged;
};

static optimizer_pass passes[] = {
#define PASS(name, level, function) { name, level, function, 0, 0 },
    OPTIMIZER_PASS_LIST(PASS)
#undef PASS
};
static const int passCount = sizeof(passes) / sizeof(passes[0]);

/* a function that still changes after this many rounds is left as it is at that point */
#define MAX_ROUNDS 16
/* how many times the whole program is scanned for calls to inline (each round can
   inline one more level of a call chain) */
#define MAX_INLINE_ROUNDS 4
/* lowest -O level at which calls are inlined */
#define INLINE_LEVEL 3
/* loop passes that run after everything else */
#define LOOP_LEVEL 2

// ---------------------------------------------------------------------------------
// Self check
// ---------------------------------------------------------------------------------

/**
 * Encode every instruction from its decoded operands and make sure the result
 * means exactly what the generator wrote. Words may differ only where the
 * encoding has two spellings of the same value (a zero operand can carry either
 * sign bit), so differing words are compared by what they decode to.
 */
static bool round_trip_is_exact(function_ir &ir, std::string &why) {
    std::vector<opcode_instr> words;
    if(!encode_words(ir, true, words, why))
        return false;

    code_info *ci = ir.ci;
    if((Int)words.size() != (Int)ci->code.size()) {
        why = "re-encoded size differs";
        return false;
    }

    Int pc = 0;
    for(ir_instruction &instr : ir.code) {
        const instruction_info &info = instruction_info_of(instr.op);
        int count = words_of(info);

        for(int w = 0; w < count; w++) {
            opcode_instr original = ci->code.get(pc + w), rebuilt = words[pc + w];
            if(original == rebuilt) continue;

            // a raw second word must match bit for bit
            bool rawWord = w == 1 && info.layout != layout_d_c;
            bool same = !rawWord && GET_OP(original) == GET_OP(rebuilt);

            if(same) {
                switch(w == 1 ? layout_c : info.layout) {
                    case layout_d: case layout_d_raw: case layout_d_c:
                        same = GET_Da(original) == GET_Da(rebuilt); break;
                    case layout_c:
                        same = GET_Ca(original) == GET_Ca(rebuilt) && GET_Cb(original) == GET_Cb(rebuilt); break;
                    case layout_b:
                        same = GET_Ba(original) == GET_Ba(rebuilt) && GET_Bb(original) == GET_Bb(rebuilt)
                               && GET_Bc(original) == GET_Bc(rebuilt); break;
                    default:
                        same = false; break; // no operands: the words had to be identical
                }
            }

            if(!same) {
                why = std::string("decode/encode mismatch on `") + info.name + "`";
                return false;
            }
        }
        pc += count;
    }

    // the self check re-encoded every instruction: forget that, nothing has changed yet
    for(ir_instruction &instr : ir.code) instr.modified = false;
    return true;
}

// ---------------------------------------------------------------------------------
// Keeping the generator's version so a function can be put back
// ---------------------------------------------------------------------------------
struct function_backup {
    std::vector<opcode_instr> code;
    std::vector<Int> lineStarts;
    std::vector<Int> tryValues;     // every pc of the try table, in a fixed order
    std::vector<Int> labelOperands;
    int stackSize, frameStackOffset;
};

static void for_each_try_pc(code_info *ci, void (*visit)(Int &value, void *ctx), void *ctx) {
    for(Int i = 0; i < (Int)ci->tryCatchTable.size(); i++) {
        try_catch_data *data = ci->tryCatchTable.get(i);
        visit(data->tryStartPc, ctx); visit(data->tryEndPc, ctx);
        visit(data->blockStartPc, ctx); visit(data->blockEndPc, ctx);

        for(Int j = 0; j < (Int)data->catchTable.size(); j++)
            visit(data->catchTable.get(j)->handlerPc, ctx);

        if(data->finallyData) {
            Int start = (Int)data->finallyData->startPc, end = (Int)data->finallyData->endPc;
            visit(start, ctx); visit(end, ctx);
            data->finallyData->startPc = (uInt)start;
            data->finallyData->endPc = (uInt)end;
        }
    }
}

static void save(code_info *ci, function_backup &backup) {
    for(Int i = 0; i < (Int)ci->code.size(); i++) backup.code.push_back(ci->code.get(i));
    for(Int i = 0; i < (Int)ci->lineTable.size(); i++) backup.lineStarts.push_back(ci->lineTable.get(i)->start_pc);
    for(Int i = 0; i < (Int)ci->labelOperands.size(); i++) backup.labelOperands.push_back(ci->labelOperands.get(i));
    for_each_try_pc(ci, [](Int &value, void *ctx) { ((function_backup*)ctx)->tryValues.push_back(value); }, &backup);
    backup.stackSize = ci->stackSize;
    backup.frameStackOffset = ci->frameStackOffset;
}

static void restore(code_info *ci, function_backup &backup) {
    ci->code.free();
    for(opcode_instr word : backup.code) ci->code.add(word);
    for(size_t i = 0; i < backup.lineStarts.size(); i++) ci->lineTable.get(i)->start_pc = backup.lineStarts[i];

    ci->labelOperands.free();
    for(Int pc : backup.labelOperands) ci->labelOperands.add(pc);

    struct cursor { function_backup *backup; size_t next; } position = { &backup, 0 };
    for_each_try_pc(ci, [](Int &value, void *ctx) {
        cursor *c = (cursor*)ctx;
        value = c->backup->tryValues[c->next++];
    }, &position);

    ci->stackSize = backup.stackSize;
    ci->frameStackOffset = backup.frameStackOffset;
}

// ---------------------------------------------------------------------------------
// Driver
// ---------------------------------------------------------------------------------

/* the optimizations that are not part of OPTIMIZER_PASS_LIST (the driver runs them itself) */
static const char *const driverPasses[] = {
    "inline calls", "box elimination", "native returns",
    "loop formula", "loop unrolling", "loop rotation", "loop registers"
};

const std::vector<std::string>& optimization_names() {
    static std::vector<std::string> names;
    if(names.empty()) {
        for(int p = 0; p < passCount; p++) names.push_back(passes[p].name);
        for(const char *name : driverPasses) names.push_back(name);
    }
    return names;
}

/**
 * Is this optimization switched on? Everything is, unless it was left out with
 * -n<name> on the command line (options.optimize_disabled), e.g.
 *     sharpc -O -nloop-rotation -nbox-elimination ...
 * Use that to find which optimization is behind a wrong result, or to measure one.
 * (The environment variable SHARP_OPT_SKIP="loop rotation,box elimination" does
 * the same without touching the command line.)
 */
static bool pass_enabled(const char *name) {
    for(const std::string &disabled : options.optimize_disabled) {
        if(disabled == name) return false;
    }

    static const char *skip = getenv("SHARP_OPT_SKIP");
    return skip == nullptr || strstr(skip, name) == nullptr;
}

/** Run the per-function passes until none of them changes the function anymore */
static void run_local_passes(function_ir &ir, int level) {
    for(int round = 0; round < MAX_ROUNDS; round++) {
        bool changed = false;

        for(int p = 0; p < passCount; p++) {
            if(passes[p].level > level || !pass_enabled(passes[p].name)) continue;

            Int before = (Int)ir.code.size();
            if(passes[p].run(ir)) {
                changed = true;
                passes[p].timesChanged++;
                passes[p].instructionsRemoved += before - (Int)ir.code.size();
            }
        }

        if(!changed) break;
        ir.changed = true;
    }
}

void optimize_bytecode() {
    // -O means "everything": the levels the passes are registered with (1..3)
    // only say how aggressive each one is, they are no longer selectable.
    int level = options.optimize_level > 0 ? 3 : 0;
    if(level <= 0)
        return;

    Int functions = 0, optimized = 0, wordsBefore = 0, wordsAfter = 0;
    std::map<std::string, Int> skipped;

    // ---- 1. decode every function the optimizer fully understands
    std::vector<std::unique_ptr<function_ir>> decoded;
    program_ir program;

    for(Int i = 0; i < (Int)compressedCompilationFunctions.size(); i++) {
        sharp_function *fn = compressedCompilationFunctions.get(i);
        if(fn->ci == NULL || fn->ci->code.size() == 0)
            continue; // native / delegate functions have no code

        functions++;
        wordsBefore += fn->ci->code.size();

        std::unique_ptr<function_ir> ir(new function_ir());
        std::string why;

        if(!decode_function(fn, *ir)) {
            size_t at = ir->failure.find(" at pc");
            skipped[at != std::string::npos ? ir->failure.substr(0, at) : ir->failure]++;
            continue;
        }

        if(!round_trip_is_exact(*ir, why)) {
            skipped[why]++;
            continue;
        }

        program.functions.push_back(ir.get());
        program.byAddress[fn->ci->address] = ir.get();
        decoded.push_back(std::move(ir));
    }

    // ---- 2. clean up every function on its own
    for(function_ir *ir : program.functions)
        run_local_passes(*ir, level);

    // ---- 3. across functions: inline small callees, then clean up what that exposed.
    // Each round can inline one more level of a chain a() -> b() -> c().
    if(level >= INLINE_LEVEL) {
        for(int round = 0; round < MAX_INLINE_ROUNDS; round++) {
            bool any = false;

            for(function_ir *ir : program.functions) {
                if(pass_enabled("inline calls") && pass_inline_calls(*ir, program)) {
                    any = true;
                    run_local_passes(*ir, level);
                }

                // inlining is what brings "create a number object" and "read the
                // number back out" into the same function
                if(pass_enabled("box elimination") && pass_box_elimination(*ir, program)) {
                    any = true;
                    run_local_passes(*ir, level);
                }
            }

            if(!any) break;
        }
    }

    // ---- 3a. calls that only unwrap the number object they get back. After
    // inlining (what is inlined needs no second version), before the loop passes
    // (the new versions have loops too).
    if(level >= INLINE_LEVEL && pass_enabled("native returns")) {
        program.nativeReturnCalls = pass_native_returns(program, decoded,
                [](function_ir &ir) { run_local_passes(ir, 3); });
    }

    // ---- 3b. loop shaping. Last, because it duplicates the loop test: the
    // passes above should see (and shrink, or remove) each loop first.
    if(level >= LOOP_LEVEL) {
        for(function_ir *ir : program.functions) {
            // a formula beats any amount of loop tuning, so this one goes first
            if(pass_enabled("loop formula") && pass_loop_closed_form(*ir)) {
                program.loopsAsFormula++;
                run_local_passes(*ir, level);
            }

            // (before rotation: it removes loops altogether, and it wants them as the generator laid them out)
            if(level >= INLINE_LEVEL && pass_enabled("loop unrolling") && pass_loop_unrolling(*ir)) {
                program.loopsUnrolled++;
                run_local_passes(*ir, level);
            }

            if(pass_enabled("loop rotation") && pass_loop_rotation(*ir)) {
                program.loopsRotated++;
                run_local_passes(*ir, level);
            }

            // (needs the rotated shape: one way in at the top, one way out at the bottom)
            if(pass_enabled("loop registers") && pass_loop_registers(*ir)) {
                program.loopsInRegisters++;
                run_local_passes(*ir, level);
            }
        }
    }

    // ---- 4. write the changed functions back, and make sure what was written
    // is still a well formed function. If not, put the generator's version back.
    for(function_ir *ir : program.functions) {
        if(!ir->changed) continue;

        sharp_function *fn = ir->fn;
        function_backup backup;
        save(fn->ci, backup);

        std::string why;
        function_ir check;
        if(!encode_function(*ir, why) || !decode_function(fn, check)) {
            restore(fn->ci, backup);
            skipped["optimized code failed verification: " + (why.empty() ? check.failure : why)]++;
            continue;
        }

        optimized++;
    }

    for(Int i = 0; i < (Int)compressedCompilationFunctions.size(); i++) {
        sharp_function *fn = compressedCompilationFunctions.get(i);
        if(fn->ci != NULL) wordsAfter += fn->ci->code.size();
    }

    if(options.optimize_report) {
        cout << "\n-- bytecode optimizer (-O) --\n";
        if(!options.optimize_disabled.empty()) {
            cout << "switched off        :";
            for(const std::string &disabled : options.optimize_disabled) cout << " [" << disabled << "]";
            cout << "\n";
        }
        cout << "functions with code : " << functions << "\n";
        cout << "functions changed   : " << optimized << "\n";
        cout << "code size           : " << wordsBefore << " -> " << wordsAfter << " words\n";

        for(int p = 0; p < passCount; p++) {
            if(passes[p].level > level) continue;
            cout << "  pass " << passes[p].name << ": changed a function " << passes[p].timesChanged
                 << " times, removed " << passes[p].instructionsRemoved << " instructions\n";
        }
        if(level >= INLINE_LEVEL)
            cout << "  pass inline calls: " << program.callsInlined << " calls replaced by the callee's body\n"
                 << "  pass box elimination: " << program.boxesRemoved << " number objects replaced by a local\n"
                 << "  pass native returns: " << program.nativeReturnCalls << " calls now get the number directly, through "
                 << program.nativeReturnVersions << " new function versions\n";

        if(level >= LOOP_LEVEL)
            cout << "  pass loop formula: " << program.loopsAsFormula << " functions had loops replaced by a formula\n";
        if(level >= INLINE_LEVEL)
            cout << "  pass loop unrolling: " << program.loopsUnrolled << " functions had loops unrolled\n";
        if(level >= LOOP_LEVEL)
            cout << "  pass loop rotation: " << program.loopsRotated << " functions had loops rotated\n"
                 << "  pass loop registers: " << program.loopsInRegisters << " functions keep loop variables in registers\n";

        if(!skipped.empty()) {
            cout << "functions left untouched:\n";
            for(auto &reason : skipped)
                cout << "  " << reason.second << " x " << reason.first << "\n";
        }
        cout << endl;
    }
}
