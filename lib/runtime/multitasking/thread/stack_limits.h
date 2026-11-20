//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_STACK_LIMITS_H
#define SHARP_STACK_LIMITS_H

#include "../../../../stdimports.h"
#include "../../memory/garbage_collector.h"

/*
 * THE THREE LIMITS OF A RUNNING PROGRAM (decided 2026-11-15)
 *
 *   the VM stack       where Sharp code keeps its locals, arguments and temporaries.
 *                      One per fiber. It is a CEILING: a fiber starts with
 *                      INITIAL_STACK_SIZE slots (fiber.h, about 3 KB) and its stack
 *                      doubles when it runs out (fiber::growStack), so a fiber that
 *                      stays shallow costs 3 KB however high the ceiling is. The
 *                      ceiling only decides how deep a program may recurse before
 *                      it gets a stack_overflow_exception.   -istack <size>
 *
 *   the thread stack   the machine's own stack, used by the VM's C++ code and by
 *                      compiled (JIT) code. One per thread, reserved by the
 *                      operating system when the thread is created.   -stack <size>
 *                      (The MAIN thread's stack is made by the operating system
 *                      before the VM runs; this number does not size it.)
 *
 *   the heap           every object. Also a ceiling.   -mem <size>
 *                      (DEFAULT_HEAP_SIZE, memory/garbage_collector.h)
 *
 * The VM stack's ceiling is 16 MB: 1,048,576 slots of 16 bytes. It was 64 KB
 * (4,096 slots), which a recursive program - a compiler walking nested code -
 * used up after about a hundred levels. A million slots is also the limit Lua
 * gives its stack (LUAI_MAXSTACK).
 *
 *      C++ comparison: `ulimit -s` / the linker's /STACK for the thread stack;
 *      the VM stack has no C++ counterpart - it is the interpreter's own array.
 */
#define INTERNAL_STACK_SIZE (MB_TO_BYTES(16) / sizeof(stack_item))
#define INTERNAL_STACK_MIN KB_TO_BYTES(4)
// 2 MB (it was 1 MB until 2026-11-20): a call in compiled code takes 32 bytes
// of the thread stack since compiled code calls compiled code directly with a
// machine frame of its own (it was 16), and a recursion must get as deep as it
// did - about 57,000 calls. The operating system only reserves these
// addresses; memory is used as far as the stack really gets.
#define STACK_SIZE MB_TO_BYTES(2)
#define STACK_MIN KB_TO_BYTES(50)
#define MEMORY_MIN KB_TO_BYTES(12)
// How much of a thread's machine stack is kept back for the VM's own C++ code
// (raising the exception, compiling a function, a collection) once compiled
// code has used the rest: see sharp_thread.nativeStackFloor.
#define NATIVE_STACK_RESERVE KB_TO_BYTES(128)
#define STACK_OVERFLOW_BUF KB_TO_BYTES(10) // VERY LARGE OVERFLOW BUFFER FOR jit STACK OFERFLOW CATCHES

extern size_t threadStackSize;
extern size_t virtualStackSize;

bool valid_stack_size(uInt stackSize);
bool valid_internal_stack_size(uInt stackSize);

void initialize_main_thread_stack(std::list<string>& appArgs);

#endif //SHARP_STACK_LIMITS_H
