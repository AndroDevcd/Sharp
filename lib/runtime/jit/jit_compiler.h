//
// The interface between the virtual machine and a JIT compiler.
//
// THERE IS NO COMPILER BEHIND IT RIGHT NOW
// ----------------------------------------
// The compiler that used to live here (jit/x64 and jit/arm) translated one
// bytecode instruction at a time and did every number operation on the x87
// unit as a double. It could not work with typed numbers (an integer or a
// decimal in a vm_cell, stdimports.h), was barely faster than the interpreter
// (2.3 ns against 2.5 ns per instruction, measured on bench.b), and has been
// removed. It is in the git history up to commit c6869deb.
//
// Its replacement works on the typed bytecode and keeps values in CPU
// registers. Until it exists `jitCompiler` is null, c_options.jit is false and
// the interpreter runs everything.
//
// WHAT THE VM NEEDS FROM A COMPILER (virtual_machine.cpp)
// -------------------------------------------------------
//   jit_try_compile(function)      calls jitCompiler->compileFunction(); on
//                                  success the function is marked
//                                  isHighFrequency and prepare_method() calls
//                                  function->jfunc->compiledCode from then on
//   jit_enter_loop(pc)             the same for a hot loop: compiled code is
//                                  entered in the middle of a function, at the
//                                  instruction task->pc points to
//   compiledCode(thread, fun, registers)
//                                  runs until the function returns, or until
//                                  it has to give control back: an exception,
//                                  a context switch, a kill (jit_return_code)
//
// Compiled code must keep the things the rest of the runtime looks at in the
// places it looks: object references in the fiber's stack slots at every safe
// point (the garbage collector scans them, memory/tracing_collector.h), and
// task->pc / fp / sp correct whenever it calls into C++.
//

#ifndef SHARP_JIT_COMPILER_H
#define SHARP_JIT_COMPILER_H

#include "../../../stdimports.h"
#include "../types/sharp_function.h"
#include "../../util/jit/asmjit/src/asmjit/core.h"

struct sharp_thread;
struct jit_compiled_function;

/** why compiled code came back */
enum jit_return_code {
    JIT_OK = 0,                    // the function returned
    JIT_EXCEPTION = 0x001,         // an exception is pending
    JIT_CONTEXT_SWITCH = 0x002,    // the fiber has to give up the thread
    JIT_KILL = 0x004               // the thread was told to stop
};

/**
 * Compiled code of one function.
 *      thread      the thread running it (all VM state hangs off it)
 *      fun         this function's record (jumpTable: where each instruction starts)
 *      registers   the VM registers of the running fiber
 */
typedef int (*jit_function_ptr)(sharp_thread* thread, jit_compiled_function* fun, vm_cell* registers);

struct jit_compiled_function {
    jit_function_ptr compiledCode;
    // Where OTHER COMPILED CODE calls this function (a direct call): no
    // arguments, everything is already in the CPU registers the backend
    // reserves. Null when the backend has no direct calls.
    void* directEntry;
    void** jumpTable;               // machine code address of each bytecode instruction, by pc
    size_t jumpTableSize;
    size_t codeSize;
    bool isCompiled;
    sharp_function* originalFunction;
};

/**
 * Records that AsmJit refused to encode an instruction. Without an error
 * handler AsmJit silently drops the instruction and the rest of the function is
 * still emitted: code that runs but does the wrong thing. A backend attaches
 * one of these and refuses to install a function when `failed` is set.
 * Build with -DJIT_DEBUG to see which instruction was rejected.
 */
class JitErrorHandler : public asmjit::ErrorHandler {
public:
    bool failed = false;
    void handleError(asmjit::Error err, const char* message, asmjit::BaseEmitter* origin) override {
        failed = true;
#ifdef JIT_DEBUG
        fprintf(stderr, "jit: assembler error: %s\n", message);
#endif
    }
};

/** A compiler for one CPU architecture. */
class JitCompiler {
public:
    virtual ~JitCompiler() = default;

    /** Compile `function`. false = it stays interpreted (never an error). */
    virtual bool compileFunction(sharp_function* function, jit_compiled_function* output) = 0;
    virtual void releaseCompiledFunction(jit_compiled_function* compiledFunc) = 0;
};

extern JitCompiler* jitCompiler;        // null: no compiler for this machine (see the top of this file)
extern recursive_mutex jit_mutex;

void init_jit_compiler();
void shutdown_jit_compiler();
bool should_compile_function(sharp_function* function);

#endif //SHARP_JIT_COMPILER_H
