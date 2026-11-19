//
// The system-interrupt dispatch table and entry point.
//
// exec_interrupt(flag) used to be a ~600 line `switch` in virtual_machine.cpp.
// It is now a table lookup: find the handler wired to this flag and call it.
// The handlers themselves live in focused topic files in this folder, one file
// per kind (console_io, time, numbers, memory_gc, threads, fibers, files,
// process, native_lib, reflection).
//
// WHY A TABLE AND NOT A SWITCH
// ----------------------------
// A `switch` keeps every case in one function - it grows without bound, and a
// new feature means surgery on a 600 line body. A table keeps one short row per
// interrupt and lets each handler live with its own kind. The cost is one
// indirect call per interrupt, which is nothing here: system interrupts are the
// SLOW path (io, threads, files), never the hot bytecode loop. The bytecode
// interpreter keeps its own switch for exactly that reason - do not table-ify
// that one.
//
// ADDING AN INTERRUPT
// -------------------
// 1. write the handler in the fitting topic file (or a new one),
// 2. declare it in interrupt_internal.h,
// 3. add a row below.
// Nothing else - no central switch to edit.
//
#include "interrupt_internal.h"
#include <array>
#include <sstream>

// One row: the interrupt's flag, its name (for tooling and messages), and the
// handler that runs it.
struct interrupt_row {
    int flag;
    const char *name;
    interrupt_fn run;
};

//
// Every system interrupt, grouped by its topic file, in the same order as the
// files - so this list doubles as a table of contents for the folder. The flag
// names are in lib/core/vm_interrupt_flag.h; the signatures (what each reads and
// writes) are in interrupt_signatures.cpp.
//
static const interrupt_row interrupt_rows[] = {
    // ---- console_io.cpp
    { OP_PRINTF,                "printf",               op_printf },
    { OP_FLUSH,                 "flush",                op_flush },
    { OP_KBHIT,                 "kbhit",                op_kbhit },
    { OP_STRTOD,                "strtod",               op_strtod },
    { OP_STRTOL,                "strtol",               op_strtol },

    // ---- time.cpp
    { OP_OS_TIME,               "os_time",              op_os_time },
    { OP_NAN_TIME,              "nano_time",            op_nano_time },
    { OP_DELAY,                 "delay",                op_delay },

    // ---- numbers.cpp
    { OP_MATH,                  "math",                 op_math },
    { OP_RANDOM_INT,            "random_int",           op_random_int },
    { OP_RANDOM_DOUBLE,         "random_double",        op_random_double },
    { OP_RANDOM_INT_RANGED,     "random_int_ranged",    op_random_int_ranged },
    { OP_RANDOM_DOUBLE_RANGED,  "random_double_ranged", op_random_double_ranged },
    { OP_RANDOM_SEED,           "random_seed",          op_random_seed },

    // ---- memory_gc.cpp
    { OP_GC_EXPLICIT,           "gc_explicit",          op_gc_explicit },
    { OP_GC_SLEEP,              "gc_sleep",             op_gc_sleep },
    { OP_GC_WAKEUP,             "gc_wakeup",            op_gc_wakeup },
    { OP_GC_KILL,               "gc_kill",              op_gc_kill },
    { OP_GC_STATE,              "gc_state",             op_gc_state },
    { OP_MEMORY_LIMIT,          "memory_limit",         op_memory_limit },
    { OP_MEMORY,                "memory",               op_memory },
    { OP_SIZEOF,                "sizeof",               op_sizeof },
    { OP_MEM_COPY,              "mem_copy",             op_mem_copy },
    { OP_INVERT,                "invert",               op_invert },
    { OP_REALLOC,               "realloc",              op_realloc },
    { OP_COPY,                  "copy",                 op_copy },
    { OP_SERIALIZE,             "serialize",            op_serialize },
    { OP_DESERIALIZE,           "deserialize",          op_deserialize },

    // ---- threads.cpp
    { OP_THREAD_START,          "thread_start",         op_thread_start },
    { OP_THREAD_JOIN,           "thread_join",          op_thread_join },
    { OP_THREAD_INTERRUPT,      "thread_interrupt",     op_thread_interrupt },
    { OP_THREAD_CREATE,         "thread_create",        op_thread_create },
    { OP_THREAD_PRIORITY,       "thread_priority",      op_thread_priority },
    { OP_THREAD_SUSPEND,        "thread_suspend",       op_thread_suspend },
    { OP_THREAD_UNSUSPEND,      "thread_unsuspend",     op_thread_unsuspend },
    { OP_THREAD_CURRENT,        "thread_current",       op_thread_current },
    { OP_THREAD_ARGS,           "thread_args",          op_thread_args },
    { OP_THREAD_SET_CURRENT,    "thread_set_current",   op_thread_set_current },
    { OP_CORES,                 "cores",                op_cores },
    { OP_YIELD,                 "yield",                op_yield },

    // ---- fibers.cpp
    { OP_FIBER_START,           "fiber_start",          op_fiber_start },
    { OP_FIBER_SUSPEND,         "fiber_suspend",        op_fiber_suspend },
    { OP_FIBER_UNSUSPEND,       "fiber_unsuspend",      op_fiber_unsuspend },
    { OP_FIBER_KILL,            "fiber_kill",           op_fiber_kill },
    { OP_FIBER_BIND,            "fiber_bind",           op_fiber_bind },
    { OP_FIBER_BOUND_COUNT,     "fiber_bound_count",    op_fiber_bound_count },
    { OP_FIBER_STATE,           "fiber_state",          op_fiber_state },
    { OP_FIBER_CURRENT,         "fiber_current",        op_fiber_current },

    // ---- files.cpp
    { OP_ABS_PATH,              "abs_path",             op_abs_path },
    { OP_CURRENT_DIRECTORY,     "current_directory",    op_current_directory },
    { OP_FILE_ACCESS,           "file_access",          op_file_path_op },
    { OP_FILE_ATTRS,            "file_attrs",           op_file_path_op },
    { OP_FILE_UPDATE_TM,        "file_update_time",     op_file_path_op },
    { OP_FILE_SIZE,             "file_size",            op_file_path_op },
    { OP_FILE_CREATE,           "file_create",          op_file_path_op },
    { OP_FILE_DELETE,           "file_delete",          op_file_path_op },
    { OP_GET_FILES,             "get_files",            op_file_path_op },
    { OP_CREATE_DIR,            "create_dir",           op_file_path_op },
    { OP_DELETE_DIR,            "delete_dir",           op_file_path_op },
    { OP_UPDATE_FILE_TM,        "update_file_time",     op_file_path_op },
    { OP_CHMOD,                 "chmod",                op_file_path_op },
    { OP_READ_FILE,             "read_file",            op_file_path_op },
    { OP_RENAME_FILE,           "rename_file",          op_file_two_paths },
    { OP_WRITE_FILE,            "write_file",           op_file_two_paths },
    { OP_DISK_SPACE,            "disk_space",           op_disk_space },

    // ---- process.cpp
    { OP_NOP,                   "nop",                  op_nop },
    { OP_SYSTEM_EXE,            "system",               op_system },
    { OP_EXIT,                  "exit",                 op_exit },
    { OP_OS_NAME,               "os_name",              op_os_name },
    { OP_PROGRAM_PATH,          "program_path",         op_program_path },
    { OP_RUNTIME_PATH,          "runtime_path",         op_runtime_path },
    { OP_C_LIBRARY,             "c_library",            op_c_library },
    { OP_C_LINK_ALL,            "c_link_all",           op_c_link_all },

    // ---- native_lib.cpp
    { OP_LOAD_LIBRARY,          "load_library",         op_load_library },
    { OP_FREE_LIBRARY,          "free_library",         op_free_library },
    { OP_LINK_FUNC,             "link_function",        op_link_func },

    // ---- reflection.cpp
    { OP_GET_FRAME_INFO,        "get_frame_info",       op_get_frame_info },
    { OP_GET_STACK_TRACE,       "get_stack_trace",      op_get_stack_trace },
    { OP_DESCRIBE_OBJECT,       "describe_object",      op_describe_object },
    { OP_REFLECT_FIELD_VALUE,   "reflect_field_value",  op_reflect_field_value },
};

//
// flag -> handler, built once on first use. Interrupt flags are small numbers
// (below 0x100, see vm_interrupt_flag.h), so a flat 256-slot array IS the whole
// "map": no hashing, no searching, just dispatch[flag].
//
// C++ note: a function-local static is initialised exactly once, and C++
// guarantees that even when several threads reach it together - so no lock and
// no init-order worry.
//
static const std::array<interrupt_fn, 256> &dispatch_table() {
    static const std::array<interrupt_fn, 256> table = [] {
        std::array<interrupt_fn, 256> t{};              // all slots null
        for(const interrupt_row &row : interrupt_rows)
            t[row.flag] = row.run;
        return t;
    }();
    return table;
}

//
// Run one system interrupt. Called from the interpreter (INT opcode) and from
// jitted code. A flag with no handler is a fault - the asm block that raised it
// asked for an interrupt that does not exist.
//
void exec_interrupt(Int interrupt) {
    const std::array<interrupt_fn, 256> &dispatch = dispatch_table();

    if(interrupt >= 0 && interrupt < 256 && dispatch[(size_t)interrupt] != nullptr) {
        dispatch[(size_t)interrupt](interrupt);
        return;
    }

    stringstream ss;
    ss << "invalid system interrupt signal: " << interrupt;
    throw vm_exception(vm.invalid_operation_except, ss.str());
}
