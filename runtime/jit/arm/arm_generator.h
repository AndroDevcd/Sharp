//
// THE ARM JIT, PART 2 OF 2: THE GENERATOR - what a compiled function does, for both ARM processors.
//
// The same map as the x86-64 JIT (jit/x64/x64_generator.h), folder for folder:
//
//      arm_compiler.h / .cpp       the JitCompiler the VM talks to: drives a generator, installs
//                                  the result (the jump table)
//      arm_generator.h / .cpp      this class: the instruction being compiled and the building
//                                  blocks every instruction uses
//      arm_machine.h               the operations all of this is written in
//      arm64/  arm32/              those operations as machine code, per processor
//
//      sections/                   the parts every compiled function has ONCE
//          entry.cpp                   the C++ door, the body's two doors
//          dispatch.cpp                the resume check and the jump through the jump table
//          exception.cpp               "an exception is pending": find the handler or leave
//          exits.cpp                   context switch / kill exits and the way out
//          slow_paths.cpp              rare code kept out of line
//
//      opcode/                     one file per KIND of instruction - the same kinds, the same
//                                  file names and the same cases as jit/x64/opcode
//
// WHAT IS THE SAME AS ON x86-64: the six reserved registers (thread, task, regs, fp, sp, ptr), the
// fiber's stack and frames, the jump table with an address for every instruction, the return codes,
// the runtime helpers (jit/jit_runtime.h) - a compiled function looks the same to the rest of the VM.
//
// Calls from compiled code to compiled code are direct, as on x86-64 (opcode/direct_calls.cpp).
//
// WHAT IS NOT: the VM registers stay in the fiber's register array (arm_machine.h says why; the
// 64-bit machine keeps copies of them in CPU registers underneath). The local numbers a function
// uses most are kept in CPU registers as far as the machine has them (pick_local_homes).
//
// Each case in opcode/ shows the interpreter's statement it stands for; when the interpreter's
// handler changes (virtual_machine.cpp, main_vm_loop), the case here and the one in jit/x64 change.
//
#pragma once

#include "arm_machine.h"
#include "../jit_compiler.h"
#include "../jit_runtime.h"
#include "../../virtual_machine.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../memory/vm_stack.h"
#include "../../memory/garbage_collector.h"
#include "../../opcode/opcode.h"
#include "../../opcode/opcode_macros.h"
#include "../../multitasking/thread_state.h"
#include "../../types/vm_register.h"
#include <vector>

// Direct calls do prepare_method()'s work in machine code. Builds where prepare_method() checks or
// counts more than that keep every call on the slow way through the runtime (as jit/x64 does).
#if !defined(SAFE_EXECUTION) && !defined(SHARP_TELEMETRY) && !defined(SHARP_TYPE_CHECK)
#define SHARP_ARM_JIT_DIRECT_CALLS
#endif

namespace armjit {

/** what an emit_<kind>() function says about the current instruction */
enum emit_result { emitted, not_mine, cannot_compile };

/** rare code that is placed after the function (sections/slow_paths.cpp) */
enum slow_kind { slow_grow, slow_null, slow_bounds, slow_safepoint, slow_barrier,
                 slow_call, slow_virtual_call, slow_found_call, slow_function_value_call };

struct slow_path_request {
    slow_kind kind;
    int entry;          // jumped to from the fast path
    int back;           // slow_grow, slow_barrier: where the fast path continues (-1: nowhere)
    Int pc;             // the instruction (slow_safepoint: the jump's target)
    void *data;         // slow_virtual_call: the place's cache
};

/** how many words an instruction takes (1 or 2) */
int instruction_length(int op);

/** the 64 bits of a decimal */
int64_t bits_of(double value);

class ArmGenerator {
public:
    ArmGenerator(ArmMachine &machine, sharp_function *fn, jit_compiled_function *record);

    /** the whole function; false = it cannot be compiled (see blockedBy) */
    bool generate();

    // ---- results, read by the compiler
    std::vector<int> labels;            // the machine code of each instruction, by pc
    std::vector<bool> starts;           // is this word the start of an instruction?
    int decline = -1;                   // "this pc is not an instruction" exit
    int directEntry = -1;               // the direct door: what other compiled code calls
    Int count;                          // number of bytecode words
    int blockedBy = -1;                 // the opcode that could not be compiled

private:
    ArmMachine &m;
    sharp_function *function;
    jit_compiled_function *record;
    uint32_t *code;

    // ---- the instruction being compiled (set by decode())
    Int pc = 0;
    int op = 0;
    Int single = 0;
    Int dual1 = 0, dual2 = 0;
    Int triple1 = 0, triple2 = 0, triple3 = 0;
    int32_t raw = 0;
    Int next = 0;

    std::vector<slow_path_request> slowPaths;

    // ---- labels of the sections
    int doorEntry = -1, resumeEntry = -1, resumeCheck = -1, dispatch = -1, exception = -1;
    int exitSwitch = -1, exitKill = -1, leave = -1;
    int callNotFinished = -1;           // after a direct call that did not return JIT_OK (sections/dispatch.cpp)

    void decode();
    bool emit_instruction();
    void pick_local_homes();

    // ---- sections/
    void emit_entry();
    void emit_dispatch();
    void emit_exception();
    void emit_exits();
    void emit_slow_paths();

    // ---- opcode/
    emit_result emit_integer_math();
    emit_result emit_decimal_math();
    emit_result emit_stack_and_locals();
    emit_result emit_objects();
    emit_result emit_fields_and_arrays();
    emit_result emit_allocation();
    emit_result emit_casts_and_locks();
    emit_result emit_control_flow();
    emit_result emit_calls();
    emit_result emit_console();
    emit_result emit_slot_forms();                              // opcode/slot_forms.cpp
    void slot_object_or_throw(Int slot, PReg to);               // ptr on the slot; to = its object
    void slot_element_or_throw(Int array, Int index, PReg to);  // ... and V1 = the index, inside the array

    // ---- opcode/direct_calls.cpp: compiled code calls compiled code
    void direct_call(Int address);                              // CALL
    void virtual_call(Int argumentCount);                       // INVOKE_DELEGATE, not static
    void function_value_call(Int argumentCount);                // CALLD
    void direct_return();                                       // RET without an exception
    void call_found_function(Int instructionWords, int slowCall, int afterCall);
    void jump_unless_room_for_a_frame(int slowCall);
    void record_the_way_back(Int lastWord);
    void run_the_callee();
    void continue_after_call(Int continueAt, int afterCall);

    // ---- where things are (arm_generator.cpp)
    static int32_t vm_register(Int n) { return (int32_t) (n * 8); }            // offset from REGS
    static int32_t slot_number(Int n) { return (int32_t) (n * 16); }           // (base+n)->var, from FP or SP
    static int32_t slot_object(Int n) { return (int32_t) (n * 16 + 8); }       // (base+n)->obj.o

    // ---- building blocks (arm_generator.cpp)
    void load_register(VReg to, Int n) { m.v_load(to, REGS, vm_register(n)); }
    void store_register(Int n, VReg from) { m.v_store(from, REGS, vm_register(n)); }
    void load_decimal(DReg to, Int n) { m.d_load(to, REGS, vm_register(n)); }
    void store_decimal(Int n, DReg from) { m.d_store(from, REGS, vm_register(n)); }
    void save_stack_registers();                    // task->sp = sp; task->ptr = ptr;
    void load_stack_registers();                    // fp = task->fp; sp = task->sp; ptr = task->ptr;
    void set_current_instruction(Int at);           // task->pc = task->rom + at;
    void prepare_runtime_call();                    // set_current_instruction(pc) + save_stack_registers()
    void call_checked(void *function, const std::vector<Arg> &arguments);   // ... and to the exception section if it raised one
    void jump_if_signal(int label);                 // if(thread->signal) goto label
    void jump_if_exception(int label);              // if(thread->signal has the exception bit) goto label
    int request_slow_path(slow_kind kind, Int at, int back = -1);
    void push_stack_slot();                         // ++sp, growing the stack when it is full
    void pop_stack_slot();                          // --sp
    void load_object_or_throw(PReg to);             // to = ptr->o, or nullptr_exception
    void load_object_or_null(PReg to);              // to = ptr ? ptr->o : null
    bool jump_to_instruction(Int target);           // (a loop safe point when backwards)
    bool before_conditional_jump(Int target);       // the safe point of a jump backwards, BEFORE its comparison; false: no such instruction
    void check_after_runtime_call(bool continuesHere);

    // ---- arithmetic on VM registers
    void integer_operation(VOp how, Int left, Int right, Int result);       // result = left op right
    void integer_operation_with_constant(VOp how, Int n, int64_t value);    // regs[n] op= value
    void integer_compare(Int left, Int right, Int out, Cond how);           // out = left ? right
    void local_operation(VOp how, Int local, Int n);                        // (fp+local)->var op= regs[n]
    void local_operation_with_constant(VOp how, Int local, int64_t value);
    void decimal_operation(DOp how, Int left, Int right, Int result);
    void decimal_operation_with_constant(DOp how, Int n, double value);
    void decimal_compare(Int left, Int right, Int out, Cond how);
    void local_decimal_operation(DOp how, Int local, Int n);
    void local_decimal_operation_with_constant(DOp how, Int local, double value);
    void remainder_into_v0(void);                   // V0 = vm_remainder(V0, V1), or the exception section
    void quotient_into_v0(void);                    // V0 = vm_quotient(V0, V1), or the exception section
};

} // namespace armjit
