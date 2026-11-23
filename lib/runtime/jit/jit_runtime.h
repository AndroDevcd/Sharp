//
// What compiled code calls back into the runtime for.
//
// Compiled code does the cheap, frequent things itself: arithmetic, moving
// numbers and references between registers, locals, the stack, fields and
// array elements, comparisons and jumps. Everything that is rare, big, or can
// fail in a way that needs a C++ exception goes through one of the functions
// below.
//
// THE RULES EVERY ONE OF THEM FOLLOWS
//
//   1. No C++ exception leaves them. Machine code has no way to unwind one, so
//      each function catches vm_exception itself and turns it into what the
//      interpreter does at the same point: the exception flag of the thread
//      (tsig_except). Compiled code tests that flag after the call.
//
//   2. They may look at the fiber. Before the call, compiled code has written
//      back what it keeps in CPU registers (task->sp, task->ptr) and has set
//      task->pc to the instruction being executed; afterwards it reads
//      task->sp / fp / ptr again, because a call, an interrupt or a growing
//      stack can have changed them.
//
//   3. They do exactly what the interpreter's handler of the same instruction
//      does (virtual_machine.cpp, main_vm_loop). When one of the two changes,
//      change the other.
//
// They are plain functions with integer / pointer arguments, so the same set
// serves every CPU backend (jit/x64 today).
//

#ifndef SHARP_JIT_RUNTIME_H
#define SHARP_JIT_RUNTIME_H

#include "../../../stdimports.h"
#include "../memory/sharp_object.h"

struct sharp_thread;
struct jit_compiled_function;

// ---- calls and returns ------------------------------------------------------------
void jit_rt_call(Int address);                              // CALL
void jit_rt_call_dynamic(Int argumentCount);                // CALLD (the function value is under the arguments)
void jit_rt_invoke_delegate(Int address, Int argSize, Int staticCall);

// A VIRTUAL CALL'S CACHE (jit/x64/opcode/direct_calls.cpp, virtual_call). Each place in compiled
// code that calls an overridden function has one: for the classes of the objects it has met, the
// function that class runs. An entry is one word, written and read whole (two threads can use a
// place at once):   [ the function's address : 32 bits | the class's address : 32 bits ]
// An empty entry is all ones, which is no class.
const int VIRTUAL_CALL_CACHE_ENTRIES = 4;
uint64_t* jit_new_virtual_call_cache();         // (lives as long as the program, like the code that reads it)
void jit_rt_invoke_virtual(Int address, Int argumentCount, uint64_t *cache);   // the slow way: look it up, remember it, call it

/**
 * The function is over (JIT_OK) or an exception is leaving it (JIT_EXCEPTION):
 * pop its VM frame like the interpreter's RET. Returns `code`.
 */
Int jit_rt_leave(sharp_thread *thread, Int code);

/** A fiber is being continued: run the frames above this one first (see fiber::rebuilding). */
void jit_rt_rebuild_above(jit_compiled_function *fun);

// ---- signals and exceptions -------------------------------------------------------
enum jit_safepoint_result {
    jit_continue = 0,       // nothing to do (any more): keep running
    jit_exception = 1,      // an exception is pending
    jit_switch = 2,         // the fiber has to give up the thread
    jit_kill = 3            // the thread was told to stop
};

/** The interpreter's `state_check:` - the thread's signal word is not zero. */
Int jit_rt_safepoint(sharp_thread *thread);

/**
 * An exception is pending and task->pc is the instruction it happened at:
 * the interpreter's `catch_exception:`.
 *      0   nothing in this function catches it (leave with JIT_EXCEPTION)
 *      1   task->pc now points at the handler: continue there
 *      2   the thread was killed
 */
Int jit_rt_catch(sharp_thread *thread);

void jit_rt_null_pointer(sharp_thread *thread);             // raise nullptr_exception
void jit_rt_out_of_bounds(sharp_thread *thread, Int index); // raise the CHECKLEN exception
void jit_rt_throw(sharp_thread *thread);                    // THROW: the exception object is on the stack

/** A push found the stack full: grow it (or raise stack overflow). */
void jit_rt_grow_stack(sharp_thread *thread);

// ---- objects ----------------------------------------------------------------------
// The new object is returned (null with the exception flag set on failure);
// compiled code pushes it.
void jit_rt_remember(sharp_object *owner, object *slot);    // POPOBJ's write barrier, the rare part
sharp_object* jit_rt_new_class(Int classAddress);
sharp_object* jit_rt_new_number_array(Int size, Int type);
sharp_object* jit_rt_new_object_array(Int size);
sharp_object* jit_rt_new_class_array(Int classAddress, Int size);
sharp_object* jit_rt_new_string(Int stringIndex);

void jit_rt_cast(Int classAddress);                         // CAST
void jit_rt_cast_number_array(Int type);                    // VARCAST
Int  jit_rt_is(Int type);                                   // IS
Int  jit_rt_lock(sharp_thread *thread);                     // LOCK: 1 = the fiber has to wait (leave)
void jit_rt_unlock(sharp_thread *thread);                   // ULOCK
void jit_rt_swap(sharp_thread *thread);                     // SWAP

// ---- numbers ----------------------------------------------------------------------
Int jit_rt_remainder(Int left, Int right);                  // % : raises on a zero divisor
Int jit_rt_decimal_to_integer(double value);                // D2I for the values the CPU instruction cannot convert
double jit_rt_power(double base, double exponent);          // **
double jit_rt_floor(double value);                          // dfloor, where the processor has no instruction for it
double jit_rt_ceil(double value);                           // dceil, the same
// dmath: function number `which` (lib/Modules/math/cmath.cpp) of a - and of b, for the functions of two
// numbers. Twice, for how arguments travel: x64 code hands over decimals only (the number as one too),
// ARM code and the interpreter the number as a whole number.
double jit_rt_math(double a, double b, double which);
double jit_rt_math_numbered(Int which, double a, double b);

// ---- everything else --------------------------------------------------------------
Int  jit_rt_interrupt(Int flag);                            // INT: 1 = the vm terminated
void jit_rt_put_integer(Int value);                         // PUT
void jit_rt_put_decimal(double value);                      // DPUT
void jit_rt_put_character(Int value);                       // PUTC
Int  jit_rt_get_character(Int echo);                        // GET
void jit_rt_sleep(Int time);                                // SLEEP

#endif //SHARP_JIT_RUNTIME_H
