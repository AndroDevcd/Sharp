//
// Shared include surface for the system-interrupt handlers.
//
// Every system interrupt (INT <flag> - see runtime/interrupts/vm_interrupt_flag.h) used
// to live as one `case` in a single ~600 line `switch` in virtual_machine.cpp.
// They now live as small handler functions in focused topic files in THIS
// folder (console_io.cpp, time.cpp, files.cpp ...), wired to their flag by the
// table in interrupt_table.cpp.
//
// A handler reaches into the VM exactly the way its old `case` did - the same
// registers, the same stack macros, the same helpers - so to keep each topic
// file short this one header pulls in the same set of VM internals that
// virtual_machine.cpp itself includes. Think of it as the handlers' shared
// "prelude": one include instead of twenty.
//
// C++ note: nothing here is GUI, threads-only, or files-only; it is the common
// bundle. A topic file includes just this and then reads like plain VM code.
//
#ifndef SHARP_INTERRUPT_INTERNAL_H
#define SHARP_INTERRUPT_INTERNAL_H

#include "../virtual_machine.h"
#include "../sni/sni_runtime.h"
#include "../error/vm_exception.h"
#include "../opcode/opcode_macros.h"
#include "../types/vm_register.h"
#include "../multitasking/thread_state.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/fiber/fiber.h"
#include "../multitasking/thread/thread_controller.h"
#include "../reflect/reflect_helpers.h"
#include "../memory/garbage_collector.h"
#include "../memory/tracing_collector.h"
#include "../memory/type_check.h"
#include "../memory/function_value.h"
#include "../memory/memory_helpers.h"
#include "../types/sharp_field.h"
#include "../console_helper.h"
#include "../util/time.h"
#include "../multitasking/fiber/task_controller.h"
#include "native/math_functions.h"
#include "native/random_numbers.h"
#include "../vm_initializer.h"
#include "../util/File.h"
#include "native/memory_copy.h"
#include "native/file_system.h"
#include "native/serialization.h"
#include "../types/access_flag.h"
#include "../termios.h"
#include "../jit/jit_compiler.h"
#include "../main.h"
#include "../telemetry/telemetry.h"

//
// A handler runs ONE system interrupt. It reads and writes the register array
// (registers[...]) and the running fiber's stack, just as the matching `case`
// did, and returns nothing.
//
// `interrupt` is the flag that was dispatched. Most handlers ignore it; a few
// serve a family of related interrupts through one body (the file handler is
// one body for OP_FILE_ACCESS .. OP_READ_FILE, told apart by the flag), which
// is why it is passed in.
//
// Java comparison: like the native method behind a System.xxx call - ordinary
// C++ reaching into the runtime, no bytecode involved.
//
typedef void (*interrupt_fn)(Int interrupt);

// ---------------------------------------------------------------------------
// Handlers, grouped by the topic file that defines them. Add a line here when
// you add a handler, and a row in interrupt_table.cpp to wire it to its flag.
// ---------------------------------------------------------------------------

// console_io.cpp - writing to and reading from the console, number parsing
void op_printf(Int interrupt);
void op_flush(Int interrupt);
void op_print_bytes(Int interrupt);
void op_kbhit(Int interrupt);
void op_strtod(Int interrupt);
void op_strtol(Int interrupt);

// time.cpp - the clock and pausing
void op_os_time(Int interrupt);
void op_nano_time(Int interrupt);
void op_delay(Int interrupt);

// numbers.cpp - math and random numbers
void op_math(Int interrupt);
void op_random_int(Int interrupt);
void op_random_double(Int interrupt);
void op_random_int_ranged(Int interrupt);
void op_random_double_ranged(Int interrupt);
void op_random_seed(Int interrupt);

// memory_gc.cpp - the garbage collector and raw memory
void op_gc_explicit(Int interrupt);
void op_gc_sleep(Int interrupt);
void op_gc_wakeup(Int interrupt);
void op_gc_kill(Int interrupt);
void op_gc_state(Int interrupt);
void op_gc_young(Int interrupt);
void op_gc_fact(Int interrupt);
void op_memory_hook(Int interrupt);
void op_memory_limit(Int interrupt);
void op_memory(Int interrupt);
void op_sizeof(Int interrupt);
void op_mem_copy(Int interrupt);
void op_invert(Int interrupt);
void op_realloc(Int interrupt);
void op_copy(Int interrupt);
void op_serialize(Int interrupt);
void op_deserialize(Int interrupt);

// threads.cpp - operating-system threads
void op_thread_start(Int interrupt);
void op_thread_join(Int interrupt);
void op_thread_interrupt(Int interrupt);
void op_thread_create(Int interrupt);
void op_thread_priority(Int interrupt);
void op_thread_suspend(Int interrupt);
void op_thread_unsuspend(Int interrupt);
void op_thread_current(Int interrupt);
void op_thread_args(Int interrupt);
void op_thread_set_current(Int interrupt);
void op_cores(Int interrupt);
void op_yield(Int interrupt);

// fibers.cpp - the VM's own lightweight tasks
void op_fiber_start(Int interrupt);
void op_fiber_suspend(Int interrupt);
void op_fiber_unsuspend(Int interrupt);
void op_fiber_kill(Int interrupt);
void op_fiber_bind(Int interrupt);
void op_fiber_bound_count(Int interrupt);
void op_fiber_state(Int interrupt);
void op_fiber_current(Int interrupt);

// files.cpp - files, directories and paths
void op_abs_path(Int interrupt);
void op_current_directory(Int interrupt);
void op_file_path_op(Int interrupt);     // OP_FILE_ACCESS .. OP_READ_FILE
void op_file_two_paths(Int interrupt);   // OP_RENAME_FILE, OP_WRITE_FILE
void op_disk_space(Int interrupt);

// process.cpp - the process and the machine
void op_nop(Int interrupt);
void op_system(Int interrupt);
void op_exit(Int interrupt);
void op_os_name(Int interrupt);
void op_signal_watch(Int interrupt);
void op_signal_take(Int interrupt);
void op_program_path(Int interrupt);
void op_runtime_path(Int interrupt);
void op_c_library(Int interrupt);
void op_c_link_all(Int interrupt);
void op_c_keep(Int interrupt);
void op_c_drop(Int interrupt);
void op_c_serve_one(Int interrupt);
void op_c_serve_start(Int interrupt);

// native_lib.cpp - native libraries (SNI)
void op_load_library(Int interrupt);
void op_free_library(Int interrupt);
void op_link_func(Int interrupt);

// reflection.cpp - looking at the stack and at objects at run time
void op_get_frame_info(Int interrupt);
void op_get_stack_trace(Int interrupt);
void op_reflect_facts(Int interrupt);
void op_reflect_text(Int interrupt);
void op_reflect_class_of(Int interrupt);
void op_reflect_find(Int interrupt);
void op_reflect_new(Int interrupt);
void op_reflect_get(Int interrupt);
void op_reflect_put(Int interrupt);
void op_reflect_spread(Int interrupt);
void op_reflect_box(Int interrupt);

#endif //SHARP_INTERRUPT_INTERNAL_H
