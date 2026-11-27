//
// The signature of every system interrupt. See interrupt_signatures.h for what
// the three strings mean, and exec_interrupt() in virtual_machine.cpp for the
// code each line describes.
//
// Most numbers an interrupt exchanges are integers (ids, counts, sizes, result
// codes, times in whole milli/nanoseconds). Decimals appear only where a
// fraction is the point: the value printed by printf, the operands and result
// of the math functions, string -> decimal, the decimal random numbers.
//

#include "interrupt_signatures.h"

static const interrupt_signature signatures[] = {
    // ---- console, numbers
    { OP_PRINTF,                "printf",               "egx:i ebx:d ecx:i",    "",             "" },
    { OP_PRINT_BYTES,           "print_bytes",          "ebx:i",                "o",            "" },
    { OP_WHOLE_TEXT,            "whole_text",           "ebx:i ecx:i",          "",             "" },
    { OP_STRTOD,                "strtod",               "",                     "o",            "ebx:d" },
    { OP_STRTOL,                "strtol",               "",                     "i o",          "ebx:i" },
    // (egx is only read by the functions of two numbers, pow, atan2 ...: not checked)
    { OP_MATH,                  "math",                 "ebx:d ecx:i",          "",             "cmt:d" },
    { OP_RANDOM_INT,            "random_int",           "",                     "",             "ebx:i" },
    { OP_RANDOM_INT_RANGED,     "random_int_ranged",    "ebx:i egx:i",          "",             "ebx:i" },
    { OP_RANDOM_DOUBLE,         "random_double",        "",                     "",             "ebx:d" },
    { OP_RANDOM_DOUBLE_RANGED,  "random_double_ranged", "ebx:d egx:d",          "",             "ebx:d" },
    { OP_RANDOM_SEED,           "random_seed",          "adx:i",                "",             "" },

    // ---- time
    { OP_OS_TIME,               "os_time",              "ebx:i",                "",             "bmr:i" },
    { OP_NAN_TIME,              "nano_time",            "",                     "",             "bmr:i" },
    { OP_DELAY,                 "delay",                "ebx:i",                "",             "" },

    // ---- garbage collector, memory
    { OP_GC_STATE,              "gc_state",             "",                     "",             "cmt:i" },
    { OP_GC_FACT,               "gc_fact",              "ebx:i",                "",             "ebx:i" },
    // (ecx, the time to wait, is only read by one of its commands: not checked)
    { OP_MEMORY_HOOK,           "memory_hook",          "ebx:i",                "",             "ebx:i" },
    { OP_MEMORY_LIMIT,          "memory_limit",         "",                     "",             "cmt:i" },
    { OP_MEMORY,                "memory",               "",                     "",             "cmt:i" },
    { OP_SIZEOF,                "sizeof",               "",                     "o",            "ebx:i" },
    { OP_COPY,                  "copy",                 "",                     "i i o",        "" },
    { OP_MEM_COPY,              "mem_copy",             "",                     "i i i o o",    "" },
    { OP_INVERT,                "invert",               "",                     "i i o",        "" },
    { OP_REALLOC,               "realloc",              "",                     "i",            "" },

    // ---- threads
    { OP_THREAD_START,          "thread_start",         "adx:i ebx:i",          "",             "cmt:i" },
    { OP_THREAD_JOIN,           "thread_join",          "adx:i",                "",             "cmt:i" },
    { OP_THREAD_INTERRUPT,      "thread_interrupt",     "adx:i",                "",             "cmt:i" },
    { OP_THREAD_CREATE,         "thread_create",        "adx:i ebx:i",          "",             "ebx:i cmt:i" },
    { OP_THREAD_PRIORITY,       "thread_priority",      "adx:i egx:i",          "",             "cmt:i" },
    { OP_THREAD_SUSPEND,        "thread_suspend",       "adx:i",                "",             "cmt:i" },
    { OP_THREAD_UNSUSPEND,      "thread_unsuspend",     "adx:i ebx:i",          "",             "cmt:i" },
    { OP_CORES,                 "cores",                "",                     "",             "ebx:i" },

    // ---- fibers
    // top first: name, the main function, the thread id, the fiber, the arguments
    // (platform/kernel/vm.sharp, start_fiber). The function was an index - "i" - until a
    // function value became an object (function_value.sharp); the kind checker then read
    // the number half of an object's slot, which holds whatever an earlier value left there
    // ("stack slot 13 holds a decimal, an integer is needed" in the coroutine cases built
    // with the bootstrap's -O, which leaves other values on that part of the stack).
    { OP_FIBER_START,           "fiber_start",          "",                     "o o i o o",    "ebx:i" },
    { OP_FIBER_SUSPEND,         "fiber_suspend",        "",                     "i",            "ebx:i" },
    { OP_FIBER_UNSUSPEND,       "fiber_unsuspend",      "",                     "i",            "ebx:i" },
    { OP_FIBER_KILL,            "fiber_kill",           "",                     "i",            "ebx:i" },
    { OP_FIBER_BIND,            "fiber_bind",           "",                     "i i",          "ebx:i" },
    { OP_FIBER_BOUND_COUNT,     "fiber_bound_count",    "",                     "i",            "ebx:i" },
    { OP_FIBER_STATE,           "fiber_state",          "",                     "i",            "ebx:i" },

    // ---- process
    { OP_SYSTEM_EXE,            "system",               "",                     "o",            "cmt:i" },
    { OP_KBHIT,                 "kbhit",                "",                     "",             "cmt:i" },
    { OP_CONSOLE_COLORS,        "console_colors",       "",                     "",             "cmt:i" },
    { OP_EXIT,                  "exit",                 "",                     "i",            "" },
    { OP_SIGNAL_WATCH,          "signal_watch",         "ebx:i ecx:i",          "",             "" },
    { OP_SIGNAL_TAKE,           "signal_take",          "ebx:i",                "",             "ebx:i" },

    // ---- files (the path is on the stack; what is in ebx depends on the call)
    { OP_FILE_ACCESS,           "file_access",          "ebx:i",                "",             "ebx:i" },
    { OP_FILE_ATTRS,            "file_attrs",           "",                     "",             "ebx:i" },
    { OP_FILE_UPDATE_TM,        "file_update_time",     "ebx:i",                "",             "ebx:i" },
    { OP_FILE_SIZE,             "file_size",            "",                     "",             "ebx:i" },
    { OP_FILE_DELETE,           "file_delete",          "",                     "",             "ebx:i" },
    { OP_CREATE_DIR,            "create_dir",           "",                     "",             "ebx:i" },
    { OP_DELETE_DIR,            "delete_dir",           "",                     "",             "ebx:i" },
    { OP_UPDATE_FILE_TM,        "update_file_time",     "ebx:i",                "",             "ebx:i" },
    { OP_CHMOD,                 "chmod",                "ebx:i egx:i ecx:i",    "",             "ebx:i" },
    { OP_RENAME_FILE,           "rename_file",          "",                     "",             "ebx:i" },
    { OP_WRITE_FILE,            "write_file",           "",                     "",             "ebx:i" },
    { OP_DISK_SPACE,            "disk_space",           "ebx:i",                "",             "ebx:i" },

    // ---- native libraries
    { OP_FREE_LIBRARY,          "free_library",         "",                     "o",            "ebx:i" },
    { OP_LINK_FUNC,             "link_function",        "",                     "o o",          "ebx:i" },
};

const interrupt_signature* find_interrupt_signature(int flag) {
    for(const interrupt_signature &signature : signatures) {
        if(signature.flag == flag)
            return &signature;
    }

    return nullptr;
}
