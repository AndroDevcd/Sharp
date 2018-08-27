//
// Created by BraxtonN on 8/9/2018.
//

#ifndef SHARP_JIT_H
#define SHARP_JIT_H

#define ASMJIT_EMBED 1

#include "../runtime/profiler.h"
#include "../../stdimports.h"
#include "../util/jit/asmjit/src/asmjit/asmjit.h"
#include "oo/Method.h"

#define jit_error_compile 1             // error compiling the source
#define jit_error_mem     304           // not enough memory
#define jit_error_ok      0             // the result you want

struct jit_ctx;

typedef void (*sjit_function)(jit_ctx *);

struct jit_func {
    sjit_function func; // real address to mapped function in memory
    int64_t serial, rAddr; // jit unique serial : reference Address to generated function
};

// convient id's for each field to access
#define jit_field_id_current 0
#define jit_field_id_registers 1
#define jit_field_id_func 2
#define jit_field_id_ir 3
#define jit_field_id_overflow 4

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
#ifdef SHARP_PROF_
#define jit_field_id_thread_tprof 9
#endif
#define jit_field_id_thread_suspendPending 10
#define jit_field_id_thread_state 11

// convient id's for each field in StackElement object
#define jit_field_id_stack_element_var 0
#define jit_field_id_stack_element_object 1

// convient id's for each field in profiler object
#define jit_field_id_profiler_totalHits 0
#define jit_field_id_profiler_starttm 1
#define jit_field_id_profiler_endtm 2
#define jit_field_id_profiler_lastHit 3

// convient id's for each field in Frame object
#define jit_field_id_frame_last 0
#define jit_field_id_frame_pc 1
#define jit_field_id_frame_sp 2
#define jit_field_id_frame_fp 3

// convient id's for each field in Frame object
#define jit_field_id_method_bytecode 0

#define offset_start(s) s
#define offset_end(e) e
#define relative_offset(obj, start, end) ((int64_t)&obj->offset_end(end)-(int64_t)&obj->offset_start(start))

#define SET_LCONST_DVAL(val) \
    if(val == 0) { \
        cc.pxor(vec0, vec0); \
    } else { \
        idx = lconsts.createConstant(cc, (double)(val)); \
        lconstMem = ptr(lconsts.getConstantLabel(idx));  \
        cc.movsd(vec0, lconstMem); \
    }
#define SET_LCONST_DVAL2(vec, val) \
    if(val == 0) { \
        cc.pxor(vec, vec0); \
    } else { \
        idx = lconsts.createConstant(cc, (double)(val)); \
        lconstMem = ptr(lconsts.getConstantLabel(idx));  \
        cc.movsd(vec, lconstMem); \
    }
#define returnFuntion() \
    cc.jmp(lbl_funcend);

#define registerParams(vec, val) \
    cc, tmp, registersReg, val, tmpMem, vec

/**
 * Ohhhh the dear stack, smh....
 * This field is required to prevent the stack from fu#....screwing me over as a result
 * of a quad word or even a double quad word being pushed to the stack. it is a bit wasteful
 * yes i understand. But I am an Android developer not a systems engineer :)
 */
#define STACK_ALIGN_OFFSET 0x10

struct jit_ctx {
    Thread* current;
    double *registers;
    Method* func; // current method we are executing only used in initalization of the call
    unsigned long long *irCount, *overflow = 0;
};

extern thread_local jit_ctx jctx;

void call(jit_ctx *, int64_t);
int compile(Method *);

void releaseMem();

#endif //SHARP_JIT_H
