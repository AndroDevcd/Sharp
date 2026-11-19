//
// The code generator of the x86-64 JIT: the object that turns ONE function's
// bytecode into machine code. x64_compiler.cpp creates one per compile and
// drives it; the pieces live in the folders next to this file.
//
//      x64_compiler.h / .cpp       the JitCompiler the VM talks to: drives a
//                                  generator, installs the result (jump table)
//      x64_generator.h / .cpp      this class: state, operands, and the small
//                                  building blocks every instruction uses
//                                  (sync / reload / push / null checks ...)
//
//      sections/                   the parts every compiled function has ONCE
//          entry.cpp                   prologue, continuing a fiber, entering mid-function
//          dispatch.cpp                the resume check and the jump through the jump table
//          exception.cpp               "an exception is pending": find the handler or leave
//          exits.cpp                   context switch / kill exits and the epilogue
//          slow_paths.cpp              rare code kept out of line: grow the stack,
//                                      null pointer, out of bounds, loop safe point
//
//      opcode/                     one file per KIND of instruction
//          integer_math.cpp            add sub mul mod, compares, bit ops, mov8..movu64
//          decimal_math.cpp            dadd dsub dmul div exp, compares, i2d / d2i, ldc
//          stack_and_locals.cpp        numbers moving between registers, locals, the stack
//          objects.cpp                 the object pointer and references
//          fields_and_arrays.cpp       getf / putf, aload* / astore*
//          allocation.cpp              newclass, newarray, newstring ...
//          casts_and_locks.cpp         cast, is, swap, lock / ulock
//          control_flow.cpp            jumps and branches
//          calls.cpp                   call, calld, invoke_delegate, ret, throw, int
//          direct_calls.cpp            the fast CALL / RET: compiled code calling
//                                      compiled code without going through C++
//          console.cpp                 put, putc, get, sleep
//
// ADDING AN INSTRUCTION: add its case to the opcode/ file of its kind (create a
// new file + emit_<kind>() for a new kind and list it in emit_instruction()).
// If it takes two words, add it to instruction_length(). Each case shows the
// interpreter's statement it stands for - when the interpreter's handler
// changes (virtual_machine.cpp, main_vm_loop) change the case too.
//
// The register plan and how compiled code is entered and left are described in
// x64_compiler.h.
//

#ifndef SHARP_X64_GENERATOR_H
#define SHARP_X64_GENERATOR_H

#include "../jit_compiler.h"
#include "../jit_runtime.h"
#include "../../virtual_machine.h"
#include "../../multitasking/thread/sharp_thread.h"
#include "../../multitasking/fiber/fiber.h"
#include "../../memory/vm_stack.h"
#include "../../memory/garbage_collector.h"
#include "../../../core/opcode/opcode.h"
#include "../../../core/opcode/opcode_macros.h"
#include "../../../core/thread_state.h"
#include "../../../core/vm_register.h"
#include "../../../util/jit/asmjit/src/asmjit/x86.h"
#include <cstddef>
#include <vector>

// Direct calls do prepare_method()'s work in machine code. Builds where
// prepare_method() checks or counts more than that keep every call on the slow
// way through the runtime.
#if !defined(SAFE_EXECUTION) && !defined(SHARP_TELEMETRY) && !defined(SHARP_TYPE_CHECK)
#define SHARP_JIT_DIRECT_CALLS
#endif

namespace x64jit {

// ---- the reserved CPU registers (callee-saved: C++ calls do not disturb them) ----
const asmjit::x86::Gp THREAD = asmjit::x86::r12;    // sharp_thread*
const asmjit::x86::Gp TASK   = asmjit::x86::r13;    // fiber*
const asmjit::x86::Gp REGS   = asmjit::x86::r14;    // vm_cell*     the VM registers
const asmjit::x86::Gp FP     = asmjit::x86::r15;    // stack_item*  copy of task->fp
const asmjit::x86::Gp SP     = asmjit::x86::rbx;    // stack_item*  copy of task->sp
const asmjit::x86::Gp PTR    = asmjit::x86::rbp;    // object*      copy of task->ptr

// ---- scratch registers, free inside one instruction ----
const asmjit::x86::Gp rax = asmjit::x86::rax, rcx = asmjit::x86::rcx, rdx = asmjit::x86::rdx;
const asmjit::x86::Xmm xmm0 = asmjit::x86::xmm0, xmm1 = asmjit::x86::xmm1;

// ---- argument registers of a C++ call (only used after save_vm_registers(), see below) ----
const asmjit::x86::Gp rdi = asmjit::x86::rdi, rsi = asmjit::x86::rsi;

// ---- THE VM REGISTERS THAT LIVE IN CPU REGISTERS ("homes") ----
// The six VM registers compiled programs use most are kept in a CPU register
// for as long as machine code runs; the other four (adx, ecf, edf, ehf) stay in
// the fiber's register array.
//
//      egx  r8        ecx  r9        cx   r10
//      bmr  r11       ebx  rsi       cmt  rdi
//
// EVERY compiled function uses the same assignment. The VM registers are
// shared by all functions of a fiber (arguments and results travel in them), so
// a direct call from compiled code to compiled code passes them along with no
// work at all.
//
// These six CPU registers are NOT preserved by C++ functions. So there are two
// states, and each piece of code is written for one of them:
//
//      REGISTER MODE   the homes hold the values; the register array is stale.
//                      All instruction code (opcode/) runs in this mode.
//      MEMORY MODE     the register array holds the values; the homes are
//                      garbage. C++ sees this mode: helpers, the interpreter,
//                      and the sections that talk to them (dispatch, exception,
//                      exits).
//
//      save_vm_registers()         register mode -> memory mode   (6 stores)
//      load_vm_registers()       memory mode -> register mode   (6 loads)
//
// A VM register holds 64 bits that are an integer or a decimal. The homes are
// general purpose registers, so integer instructions use them directly and
// decimal instructions move the bits to an XMM register and back (movq).
const int HOME_COUNT = 6;
const int HOME_VM_REGISTER[HOME_COUNT] = { EGX, ECX, CX, BMR, EBX, CMT };
const asmjit::x86::Gp HOME_CPU_REGISTER[HOME_COUNT] = {
    asmjit::x86::r8, asmjit::x86::r9, asmjit::x86::r10, asmjit::x86::r11, asmjit::x86::rsi, asmjit::x86::rdi
};

// ---- our slot in the machine frame (sections/entry.cpp) ----
const int FRAME_SCRATCH = 0;        // a value kept across a helper call
const int FRAME_SIZE = 8;           // keeps rsp a multiple of 16 at our calls

/** what an emit_<kind>() function says about the current instruction */
enum emit_result {
    emitted,            // machine code written
    not_mine,           // not an instruction of this kind: ask the next file
    cannot_compile      // of this kind, but not compilable (the function stays interpreted)
};

/** rare code that is placed after the function (sections/slow_paths.cpp) */
enum slow_kind { slow_grow, slow_null, slow_bounds, slow_safepoint, slow_call, slow_barrier };

struct slow_path_request {
    slow_kind kind;
    asmjit::Label entry;    // jumped to from the fast path
    asmjit::Label back;     // slow_grow, slow_call: where the fast path continues
    Int pc;                 // the instruction (slow_safepoint: the jump's target)
};

/** how many words an instruction takes (1 or 2) */
int instruction_length(int op);

/** the 64 bits of a decimal, to load it as an immediate */
uint64_t bits_of(double value);

class X64Generator {
public:
    X64Generator(asmjit::x86::Assembler &assembler, sharp_function *fn, jit_compiled_function *record);

    /** the whole function; false = it cannot be compiled (see blockedBy) */
    bool generate();

    // ---- results, read by X64Compiler ----
    std::vector<asmjit::Label> labels;  // machine code of each instruction, by pc
    std::vector<bool> starts;           // is this word the start of an instruction?
    asmjit::Label decline;              // "this pc is not an instruction" exit
    asmjit::Label directEntry;          // where compiled code calls this function
    asmjit::Label declineFromRegisters; // what the jump table holds for a word that is no instruction
    Int count;                          // number of bytecode words
    int blockedBy = -1;                 // the opcode that could not be compiled

private:
    asmjit::x86::Assembler &a;
    sharp_function *function;
    jit_compiled_function *record;      // where the result goes (dispatch reads its jump table)
    uint32_t *code;                     // the bytecode

    // ---- the instruction being compiled (set by decode()) ----
    Int pc = 0;
    int op = 0;
    Int single = 0;                             // single_arg
    Int dual1 = 0, dual2 = 0;                   // dual_arg1, dual_arg2
    Int triple1 = 0, triple2 = 0, triple3 = 0;  // triple_arg1..3
    int32_t raw = 0;                            // raw_arg2 (the second word)
    Int next = 0;                               // pc of the instruction after it

    std::vector<slow_path_request> slowPaths;

    // ---- labels of the sections ----
    // entered in memory mode: resumeEntry, resumeCheck, dispatch, exception,
    //                         exitSwitch, exitKill, leave
    // entered in register mode: directEntry, epilogue, every instruction
    asmjit::Label resumeEntry, resumeCheck, dispatch, exception, exitSwitch, exitKill, leave, epilogue;
    // register mode -> save_vm_registers() -> a memory mode section
    asmjit::Label resumeFromRegisters;
    // a direct call came back with something other than JIT_OK
    asmjit::Label callNotFinished;

    void decode();
    bool emit_instruction();

    // ---- sections/ ----
    void emit_entry();
    void emit_dispatch();
    void emit_exception();
    void emit_exits();
    void emit_slow_paths();

    // ---- opcode/ ----
    emit_result emit_integer_math();
    emit_result emit_decimal_math();
    emit_result emit_stack_and_locals();
    emit_result emit_objects();
    emit_result emit_fields_and_arrays();
    emit_result emit_allocation();
    emit_result emit_casts_and_locks();
    emit_result emit_control_flow();
    emit_result emit_calls();
    void direct_call(Int address);      // opcode/direct_calls.cpp
    void direct_return();
    emit_result emit_console();

    // ---- VM registers (x64_generator.cpp) ----
    bool has_cpu_register(Int vmRegister);
    asmjit::x86::Gp cpu_register_of(Int vmRegister);                               // its CPU register (has_home only)
    asmjit::x86::Mem vm_register_in_memory(Int vmRegister);                       // its place in the register array
    asmjit::Operand vm_register(Int vmRegister);                                // home or memory, for a.emit()
    asmjit::x86::Gp vm_register_value(Int vmRegister, const asmjit::x86::Gp &scratch);     // the value in A register: its home, or loaded into scratch
    void load_vm_register(const asmjit::x86::Gp &to, Int vmRegister);               // to = regs[n]
    void store_vm_register(Int vmRegister, const asmjit::x86::Gp &from);            // regs[n] = from
    void store_vm_register_constant(Int vmRegister, uint64_t bits);                 // regs[n] = bits
    asmjit::x86::Xmm vm_register_decimal(Int vmRegister, const asmjit::x86::Xmm &scratch);  // the decimal, in scratch
    void store_vm_register_decimal(Int vmRegister, const asmjit::x86::Xmm &from);   // regs[n].d = from
    void save_vm_registers();                                                       // register mode -> memory mode
    void load_vm_registers();                                                     // memory mode -> register mode

    // ---- other operands ----
    asmjit::x86::Mem local_number(Int n);              // (fp+n)->var
    asmjit::x86::Mem local_object(Int n);        // (fp+n)->obj.o
    asmjit::x86::Mem stack_number(Int n = 0);            // (sp+n)->var
    asmjit::x86::Mem stack_object(Int n = 0);      // (sp+n)->obj.o
    asmjit::x86::Mem fiber_field(size_t offset);       // a field of the fiber
    asmjit::x86::Mem thread_signal();                  // thread->signal
    asmjit::x86::Mem machine_frame_slot(int slot);           // FRAME_SCRATCH

    // ---- building blocks (x64_generator.cpp) ----
    void save_stack_registers();                    // task->sp = sp; task->ptr = ptr;
    void load_stack_registers();                  // fp = task->fp; sp = task->sp; ptr = task->ptr;
    void set_current_instruction(Int at);          // task->pc = task->rom + at;
    void call_runtime(void *target);        // call a C++ function (arguments already placed)
    void call_runtime_checked(void *target);// ... and go to the exception section if it raised one
    void prepare_runtime_call();          // set_current_instruction(pc) + save_stack_registers() + save_vm_registers(): what a jit_rt_ helper relies on
    asmjit::Label request_slow_path(slow_kind kind, Int at, asmjit::Label back = asmjit::Label());
    void push_stack_slot();                    // ++sp, growing the stack when it is full
    void pop_stack_slot();                     // --sp
    void load_object_or_throw();           // rax = ptr->o, or nullptr_exception
    void load_object_or_null();   // rax = ptr ? ptr->o : null
    void load_decimal_constant(double value);
    bool jump_to_instruction(Int target);       // jmp to an instruction (loop safe point when backwards)
    void jump_to_computed_instruction();             // goto the instruction whose pc is in rax
    void check_after_runtime_call(bool continuesHere);     // after jit_rt_call & co (memory mode)

    // ---- arithmetic on VM registers ----
    void integer_operation(asmjit::InstId inst, Int left, Int right, Int result);          // result = left op right
    void integer_compare(Int left, Int right, Int out, void (X64Generator::*set)());    // out = left ? right
    void decimal_operation(asmjit::InstId inst, Int left, Int right, Int result);
    void decimal_operation_with_constant(asmjit::InstId inst, Int vmRegister, double value);    // regs[n] op= value
    void decimal_compare(Int left, Int right);                                          // the flags of left ? right

    // regs[out] = (flags) ? 1 : 0   after a comparison
    void store_condition(Int out, void (X64Generator::*set)());
    void condition_less();  void condition_greater();  void condition_less_or_equal(); void condition_greater_or_equal();
    void condition_equal();  void condition_not_equal(); void condition_above();  void condition_above_or_equal();
    void condition_decimal_equal();           // == of decimals (a NaN is not equal)
    void condition_decimal_not_equal();          // != of decimals (a NaN is not equal)
};

} // namespace x64jit

#endif //SHARP_X64_GENERATOR_H
