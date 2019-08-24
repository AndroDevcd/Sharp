//
// Created by braxtonn on 8/21/2019.
//

#include "JitAssembler.h"
#include "Thread.h"
#include "Manifest.h"
#include "main.h"
#include "Environment.h"
#include "Jit.h"

void JitAssembler::shutdown() {

    for(int64_t i = 0; i < functions.len; i++) {
        rt.release(functions.get(i));
    }
    functions.free();
}

void JitAssembler::initialize() {
    setupContextFields();
    setupThreadFields();
    setupStackElementFields();
    setupFrameFields();
    setupMethodFields();
    setupSharpObjectFields();
}

void JitAssembler::setupContextFields() {
    Thread *self = Thread::getThread(main_threadid);
    Ljit_context[jit_context_self] = getMemPtr(relative_offset((self->jctx), self, self));
    Ljit_context[jit_context_regs] = getMemPtr(relative_offset((self->jctx), self, regs));
    Ljit_context[jit_context_caller] = getMemPtr(relative_offset((self->jctx), self, caller));
}

void JitAssembler::setupThreadFields() {
    Thread* thread = Thread::getThread(main_threadid);
    Lthread[thread_calls] = getMemPtr(relative_offset(thread, calls, calls));
    Lthread[thread_dataStack] = getMemPtr(relative_offset(thread, calls, dataStack));
    Lthread[thread_sp] = getMemPtr(relative_offset(thread, calls, sp));
    Lthread[thread_fp] = getMemPtr(relative_offset(thread, calls, fp));
    Lthread[thread_current] = getMemPtr(relative_offset(thread, calls, current));
    Lthread[thread_callStack] = getMemPtr(relative_offset(thread, calls, callStack));
    Lthread[thread_stack_lmt] = getMemPtr(relative_offset(thread, calls, stack_lmt));
    Lthread[thread_cache] = getMemPtr(relative_offset(thread, calls, cache));
    Lthread[thread_pc] = getMemPtr(relative_offset(thread, calls, pc));
    Lthread[thread_state] = getMemPtr(relative_offset(thread, calls, state));
    Lthread[thread_signal] = getMemPtr(relative_offset(thread, calls, signal));
    Lthread[thread_stbase] = getMemPtr(relative_offset(thread, calls, stbase));
    Lthread[thread_stack] = getMemPtr(relative_offset(thread, calls, stack));
}

void JitAssembler::setupStackElementFields() {
    StackElement stack;
    Lstack_element[stack_element_var] = getMemPtr(relative_offset((&stack), var, var));
    Lstack_element[stack_element_object] = getMemPtr(relative_offset((&stack), var, object));
}

void JitAssembler::setupFrameFields() {
    Frame frame(0,0,0,0,false);
    Lframe[frame_last] = getMemPtr(relative_offset((&frame), last, last));
    Lframe[frame_pc] = getMemPtr(relative_offset((&frame), last, pc));
    Lframe[frame_sp] = getMemPtr(relative_offset((&frame), last, sp));
    Lframe[frame_fp] = getMemPtr(relative_offset((&frame), last, fp));
}

void JitAssembler::setupMethodFields() {
    Method m;
    Lmethod[method_bytecode] = getMemPtr(relative_offset((&m), jit_labels, bytecode));
}

void JitAssembler::setupSharpObjectFields() {
    SharpObject o;
    Lsharp_object[sharp_object_HEAD] = getMemPtr(relative_offset((&o), HEAD, HEAD));
    Lsharp_object[sharp_object_node] = getMemPtr(relative_offset((&o), HEAD, node));
    Lsharp_object[sharp_object_k] = getMemPtr(relative_offset((&o), HEAD, k));
    Lsharp_object[sharp_object_size] = getMemPtr(relative_offset((&o), HEAD, size));
}

int JitAssembler::performInitialCompile() {
    int error;
    for(int64_t i = 0; i < manifest.methods; i++) {
        if(c_options.jit && (c_options.slowBoot || env->methods[i].isjit)) {
            // we want to compile it
            env->methods[i].isjit=false;
            error = tryJit(env->methods+i);
            switch (error) {
                case jit_error_compile:
                    env->methods[i].jitAttempts = JIT_MAX_ATTEMPTS;
                    break;
                case jit_error_mem:
                    return error;
                default:
                    /* ignore */
                    break;
            }
        }
    }

    return jit_error_ok;
}

int JitAssembler::tryJit(Method* func) {
    int error = jit_error_ok;
    if(!func->isjit && func->jitAttempts < JIT_MAX_ATTEMPTS)
    {
        if((error = compile(func)) != jit_error_ok)
        {
            func->jitAttempts++;
        }
    } else if(func->jitAttempts >= JIT_MAX_ATTEMPTS)
        error = jit_error_max_attm;

    return error;
}

int JitAssembler::compile(Method *func) {
    // TODO: write
    func->compiling = false;
    return jit_error_compile;
}
