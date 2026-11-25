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
// The two VM registers compiled programs still use most are kept in a CPU
// register for as long as machine code runs; the other eight stay in the
// fiber's register array.
//
//      ebx  rsi       cmt  rdi
//
// (There were six, with egx, ecx, cx and bmr in r8 .. r11. Since the compiler
// writes the slot forms - opcode/slot_forms.cpp - a function's numbers live in
// its frame slots and hardly pass through VM registers; r8 .. r11 now hold the
// slots a function uses most: "LOCAL NUMBERS IN CPU REGISTERS" in x64_generator.cpp.)
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
const int HOME_COUNT = 2;
const int HOME_VM_REGISTER[HOME_COUNT] = { EBX, CMT };
const asmjit::x86::Gp HOME_CPU_REGISTER[HOME_COUNT] = { asmjit::x86::rsi, asmjit::x86::rdi };

// ---- the homes of a function's own numbers (frame slots): four whole numbers, four decimals ----
const int SLOT_HOME_COUNT = 4;
// (decimals have more registers to live in where the system lets a function use them freely: on
//  Windows xmm6 and up belong to the caller, and saving them would cost every entry and exit)
#ifdef _WIN32
const int DECIMAL_HOME_COUNT = 4;
#else
const int DECIMAL_HOME_COUNT = 10;
#endif

// ---- our slot in the machine frame (sections/entry.cpp) ----
// (Three slots: with the return address a direct call takes 32 bytes of the thread's machine
//  stack, and rsp stays a multiple of 16 at our calls. Every slot more costs recursion depth.)
const int FRAME_SCRATCH = 0;        // a value kept across a helper call (opcode/allocation.cpp) ...
const int FRAME_CALLEE = 0;         // ... or the function a virtual or function-value call found, while it runs: never both at once
const int FRAME_DIRECT = 8;         // 1: entered by a direct call - the caller puts its own state back (opcode/direct_calls.cpp)
const int FRAME_FP_BELOW_SP = 16;   // how far our fp is below sp, while a function we called directly runs
const int FRAME_SIZE = 24;

/** what an emit_<kind>() function says about the current instruction */
enum emit_result {
    emitted,            // machine code written
    not_mine,           // not an instruction of this kind: ask the next file
    cannot_compile      // of this kind, but not compilable (the function stays interpreted)
};

/** rare code that is placed after the function (sections/slow_paths.cpp) */
enum slow_kind { slow_grow, slow_null, slow_bounds, slow_safepoint, slow_call, slow_virtual_call, slow_found_call,
                 slow_function_value_call, slow_barrier };

struct slow_path_request {
    slow_kind kind;
    asmjit::Label entry;    // jumped to from the fast path
    asmjit::Label back;     // slow_grow, slow_call: where the fast path continues
    Int pc;                 // the instruction (slow_safepoint: the jump's target)
    void *data = nullptr;   // slow_virtual_call: the place's cache
    Int objectSlot = -1;    // slow_null, slow_bounds of a slot form: the frame slot whose object it was
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
    asmjit::Label doorEntry;            // the same door for the C++ door: the function then returns the long way
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
    bool emit_new_class_inline(Int classAddress, asmjit::Label slow, asmjit::Label done);     // opcode/allocation.cpp

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
    void virtual_call(Int argumentCount);
    void function_value_call(Int argumentCount);
    void call_function_in_rax(Int instructionWords, const asmjit::Label &slowCall, const asmjit::Label &afterCall);
    void direct_return();
    emit_result emit_console();
    emit_result emit_slot_forms();      // opcode/slot_forms.cpp
    void slot_jump_integers(int how, Int left, Int right, bool when, const asmjit::Label &to);
    void slot_jump_decimals(int how, Int left, Int right, bool when, const asmjit::Label &to);
    void slot_jump_integer_constant(int how, Int slot, Int constant, bool when, const asmjit::Label &to);
    void slot_jump_decimal_constant(int how, Int slot, Int constant, bool when, const asmjit::Label &to);
    void jump_after_integer_compare(int how, bool when, const asmjit::Label &to);   // after `cmp left, right`
    void jump_after_decimal_compare(int how, bool when, const asmjit::Label &to);   // after `ucomisd`, see slot_jump_decimals
    void slot_object_or_throw(Int slot);                // ptr on the slot, rax = its object
    void slot_element_or_throw(Int array, Int index);   // ... and rcx = the index, inside the array

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
    void load_vm_registers();                                                     // memory mode -> register mode (and the slot homes)
    void load_vm_registers_only();                                                // ... when the frame is no longer ours (leaving)

    // ---- LOCAL NUMBERS IN CPU REGISTERS: the slot homes (x64_generator.cpp) ----
    Int wholeHomes[SLOT_HOME_COUNT];        // the slots in r8 .. r11
    int wholeHomeCount = 0;
    Int decimalHomes[DECIMAL_HOME_COUNT];   // the slots in xmm2 .. xmm5 (.. xmm11 where there are ten)
    int decimalHomeCount = 0;
    void pick_slot_homes();                 // which slots, from the bytecode
    int whole_home(Int slot);               // which home (0 .. 3), -1: none
    int decimal_home(Int slot);
    asmjit::x86::Gp whole_home_register(int home);
    asmjit::x86::Xmm decimal_home_register(int home);
    void load_slot_homes();                 // every home = its slot (after C++ ran, or with a new fp)
    void sync_slot_home(Int slot);          // one home = its slot (after a write that went to memory only)
    asmjit::x86::Gp slot_in_register(Int slot, const asmjit::x86::Gp &scratch);            // to READ: its home, or loaded into scratch
    asmjit::Operand slot_operand(Int slot);                                                // its whole-number home, or memory
    asmjit::x86::Xmm slot_in_decimal_register(Int slot, const asmjit::x86::Xmm &scratch);
    asmjit::Operand slot_decimal_operand(Int slot);                                        // its decimal home, or memory
    asmjit::x86::Gp whole_result_register(Int slot, Int unlessSlot = -1);                  // where to compute a result for `slot`: its home, or rax
    asmjit::x86::Xmm decimal_result_register(Int slot, Int unlessSlot = -1);              // ... its decimal home, or xmm0
    void store_slot(Int slot, const asmjit::x86::Gp &from);                                // memory and home
    void store_slot_decimal(Int slot, const asmjit::x86::Xmm &from);
    void store_slot_constant(Int slot, uint64_t bits);                                     // (uses rax)

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
    void divide_for_remainder(const asmjit::Label &zero);     // rdx = rax % rcx the way vm_remainder gives it; to `zero` when rcx is 0
    void divide_for_quotient(const asmjit::Label &zero);        // rax = rax / rcx the way vm_quotient gives it
    bool constant_remainder(int64_t divisor);                 // rdx = rax % divisor; false: not here (0 - the runtime raises)
    void prepare_runtime_call();          // set_current_instruction(pc) + save_stack_registers() + save_vm_registers(): what a jit_rt_ helper relies on
    asmjit::Label request_slow_path(slow_kind kind, Int at, asmjit::Label back = asmjit::Label());
    void push_stack_slot();                    // ++sp, growing the stack when it is full
    void pop_stack_slot();                     // --sp
    void load_object_or_throw();           // rax = ptr->o, or nullptr_exception
    void load_object_or_null();   // rax = ptr ? ptr->o : null
    void load_decimal_constant(double value);
    bool jump_to_instruction(Int target);       // jmp to an instruction (loop safe point when backwards)
    bool before_conditional_jump(Int target);   // the safe point of a jump backwards, BEFORE its comparison; false: no such instruction
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
