//
// Created by BraxtonN on 8/9/2018.
//

#ifndef SHARP_JIT_H
#define SHARP_JIT_H

#define ASMJIT_EMBED 1

#include "../../stdimports.h"
#include "../util/jit/asmjit/src/asmjit/asmjit.h"
#include "oo/Method.h"

#define jit_error_compile 1             // error compiling the source
#define jit_error_mem     304           // not enough memory
#define jit_error_ok      0             // the result you want

struct jit_ctx;
class VirtualMachine;
class Environment;

typedef void (*sjit_function)(jit_ctx *);

struct jit_func {
    sjit_function func; // real address to mapped function in memory
    int64_t serial, rAddr; // jit unique serial : reference Address to generated function
};

// convient id's for each field to access
#define jit_field_id_current 0
#define jit_field_id_registers 1
#define jit_field_id_vm 2
#define jit_field_id_env 3
#define jit_field_id_func 4

// convient id's for each field in thread object
#define jit_field_id_thread_current 0
#define jit_field_id_thread_callStack 1
#define jit_field_id_thread_calls 2
#define jit_field_id_thread_dataStack 3
#define jit_field_id_thread_sp 4
#define jit_field_id_thread_fp 5
#define jit_field_id_thread_stack_lmt 6
#define jit_field_id_thread_cache 7
#define jit_field_id_thread_pc 8

// convient id's for each field in StackElement object
#define jit_field_id_stack_element_var 0
#define jit_field_id_stack_element_object 1

#define SIZE(x) (int64_t)(sz = (int64_t)(x))

#define SET_LCONST_DVAL(val) \
    if(val == 0) { \
        cc.pxor(vec0, vec0); \
    } else { \
        idx = lconsts.createConstant(cc, (double)(val)); \
        lconstMem = ptr(lconsts.getConstantLabel(idx));  \
        cc.movsd(vec0, lconstMem); \
    }

struct jit_ctx {
    Thread* current;
    double *registers;
    VirtualMachine* vm;
    Environment* env;
    Method* func; // current method we are executing only used in initalization of the call
};

extern thread_local jit_ctx jctx;

void call(jit_ctx *, int64_t);
int compile(Method *);

void releaseMem();

#endif //SHARP_JIT_H
