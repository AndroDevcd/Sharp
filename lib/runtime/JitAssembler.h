//
// Created by braxtonn on 8/21/2019.
//

#ifndef SHARP_JITASSEMBLER_H
#define SHARP_JITASSEMBLER_H

#define ASMJIT_EMBED 1

#include "../util/jit/asmjit/src/asmjit/asmjit.h"
#include "List.h"

using namespace asmjit;


class Thread;
class Method;

struct jit_context {
    Thread* self;
    double *regs;
    Method* caller; // current method we are executing only used in initalization of the call
};

typedef void (*fptr)(jit_context *);

// Jit helpers for field addressing
#define offset_start(s) s
#define offset_end(e) e
#define relative_offset(obj, start, end) ((int64_t)&obj->offset_end(end)-(int64_t)&obj->offset_start(start))

class JitAssembler {
public:
    JitAssembler()
    :
        rt()
    {
        functions.init();
    }

    void shutdown();
    int performInitialCompile();
    int tryJit(Method*);

protected:
    void initialize();

private:
    virtual X86Mem getMemPtr(int64_t addr) = 0;

    void setupContextFields();
    void setupThreadFields();
    void setupStackElementFields();
    void setupFrameFields();
    void setupMethodFields();
    void setupSharpObjectFields();
    int compile(Method*);

    JitRuntime rt;
    _List<fptr> functions;
    X86Mem Ljit_context[3];     // memory layout of struct jit_context {}
    X86Mem Lthread[13];         // memory layout of class Thread {}
    X86Mem Lstack_element[2];   // memory layout of struct StackElement {}
    X86Mem Lframe[4];           // memory layout of struct Frame {}
    X86Mem Lmethod[1];          // memory layout of struct Method {}
    X86Mem Lsharp_object[4];    // memory layout of struct SharpObject {}
};


// struct jit_context {} fields
#define jit_context_self   0
#define jit_context_regs   1
#define jit_context_caller 2

// class Thread {} fields
#define thread_current   0
#define thread_callStack 1
#define thread_calls     2
#define thread_dataStack 3
#define thread_sp        4
#define thread_fp        5
#define thread_stack_lmt 6
#define thread_cache     7
#define thread_pc        8
#define thread_state     9
#define thread_signal    10
#define thread_stbase    11
#define thread_stack     12

// struct StackElement {} fields
#define stack_element_var    0
#define stack_element_object 1

// struct Frame {} fields
#define frame_last 0
#define frame_pc   1
#define frame_sp   2
#define frame_fp   3

// struct Method {} fields
#define method_bytecode 0

// struct SharpObject {} fields
#define sharp_object_HEAD 0
#define sharp_object_node 1
#define sharp_object_k    2
#define sharp_object_size 3

// handy macros to use
#define JIT_MAX_ATTEMPTS 3

#endif //SHARP_JITASSEMBLER_H
