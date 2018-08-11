//
// Created by BraxtonN on 8/9/2018.
//

#ifndef SHARP_JIT_H
#define SHARP_JIT_H

#define ASMJIT_EMBED 1

#include "../../stdimports.h"
#include "../util/jit/asmjit/src/asmjit/asmjit.h"
#include "oo/Method.h"

struct jit_ctx;
class VirtualMachine;
class Environment;

typedef void (*sjit_function)(jit_ctx *);

struct jit_func {
    sjit_function func; // real address to mapped function in memory
    int64_t serial, rAddr; // jit unique serial : reference Address to generated function
};

struct jit_ctx {
    Thread* current;
    double *registers;
    VirtualMachine* vm;
    Environment* env;
    Method* func; // current method we are executing only used in initalization of the call
};

extern thread_local jit_ctx* jctx;

void call(jit_ctx *, int64_t);
int compile(Method *);

void releaseMem();

#endif //SHARP_JIT_H
