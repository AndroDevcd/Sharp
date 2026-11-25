//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_FUNCTION_H
#define SHARP_SHARP_FUNCTION_H

#include "../stdimports.h"
#include "data_type.h"
#include "function_type.h"
#include "line_data.h"
#include "try_catch_data.h"
#include "../util/linked_list.h"
#include "../sni/sni.h"

struct sharp_class;
struct sharp_type;
struct sharp_object;
struct jit_compiled_function;

struct function_param {
    sharp_type *type;
    bool isArray;
};

/**
 * The name of one local variable, as an executable built with -dbg carries it
 * (the compiler: backend/finalizer/output/debug_locals.sharp, which says what
 * each number means). The VM itself never asks for these: they are for the
 * debugger (debugger).
 */
enum local_kind {
    LOCAL_OBJECT = 0,           // an object or an array (or null)
    LOCAL_DECIMAL = 1,
    LOCAL_BOOL = 2,
    LOCAL_INT8 = 3,
    LOCAL_INT16 = 4,
    LOCAL_INT32 = 5,
    LOCAL_INT64 = 6,
    LOCAL_UINT8 = 7,
    LOCAL_UINT16 = 8,
    LOCAL_UINT32 = 9,
    LOCAL_UINT64 = 10,
    LOCAL_BOOL_ARRAY = 11,      // an array of bools
    LOCAL_TEXT = 12,            // a `string` (or null)
    LOCAL_KIND_MASK = 0xff,     // (a local's kind is one of the above, with this flag perhaps added:)
    LOCAL_OF_LOOP_HEAD = 0x100  // the variable of a `for` / `foreach` head: its line runs again on every turn
};

struct local_data {
    string name;
    int32_t kind;               // a local_kind, perhaps with LOCAL_OF_LOOP_HEAD added
    int32_t firstLine;          // the line it is declared on; 0: it is there from the function's start (`self`, a parameter)
    int32_t lastLine;           // the last line it can be named on; 0: the function's end

    // WHERE IT LIVES
    //   field -1:    in the function's frame, at `slot`
    //   field >= 0:  in a closure record (a lambda uses it): read `slot` of the frame, then for each
    //                step that field of the record reached so far; the variable is `field` of the last
    //   field -2:    a `localized` variable: each fiber has its own, at the bottom of the fiber's
    //                stack - `slot` counted from there
    int32_t slot;
    int32_t *steps;
    int32_t stepCount;
    int32_t field;
};

struct sharp_function {
    int32_t address;
    int32_t guid;
    string name;
    string fullName;
    sharp_class *owner;
    uint32_t flags;
    uint32_t* bytecode;
    function_type fnType;
    int stackSize;
    int paramSize;
    sharp_type* returnType;
    bool arrayReturn;
    long sourceFile;
    int32_t bytecodeSize;
    long delegateAddress;
    int spOffset;
    int fpOffset;
    int frameStackOffset;
    bool nativeFunc;
    // a `native` function's C++ side: the glue of its entry in a library's table (sni/sni_runtime.h);
    // null until the function is first called and found
    sni_glue nativeGlue;
    // a function of an `extern` block: which C function it is (cffi/c_bind.h); null for every other function
    struct c_binding *externC;
    function_param *params;
    linkedlist<line_data> lineTable;
    linkedlist<try_catch_data> tryCatchTable;
    local_data *locals;         // the names of its local variables; null when the executable carries none (no -dbg, or -O)
    int32_t localCount;
    uint32_t callCount;
    uint32_t loopCount;      // backward jumps taken in the interpreter (hot loop detection for the jit)
    bool isHighFrequency;
    jit_compiled_function *jfunc;
};

void init_struct(sharp_function *sf);
string get_info(sharp_function *sf, Int pc);

#endif //SHARP_SHARP_FUNCTION_H
