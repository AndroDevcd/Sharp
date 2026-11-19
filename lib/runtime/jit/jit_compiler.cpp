//
// See jit_compiler.h for the interface. This file picks the compiler for the
// CPU the runtime was built for:
//
//      x86-64 (Linux, macOS on Intel, Windows)     jit/x64/x64_compiler.cpp
//      anything else                               none: everything is interpreted
//
// (Windows passes arguments to C++ in other registers than Linux does: the two
// places that touch that are call_runtime in x64_generator.cpp and the C++ door
// in sections/entry.cpp.)
//
// A runtime built with -DSHARP_TYPE_CHECK (the number kind checker,
// memory/type_check.h) has no compiler either: the checker's tags are kept by
// the interpreter's handlers, machine code does not keep them.
//
// A new CPU gets its own folder next to jit/x64 and one more branch here.
//

#include "jit_compiler.h"
#include "../main.h"

#if defined(SHARP_HAS_ASMJIT) && !defined(SHARP_TYPE_CHECK)
#define SHARP_JIT_X64
#include "x64/x64_compiler.h"
#endif

JitCompiler* jitCompiler = nullptr;
recursive_mutex jit_mutex;

void init_jit_compiler() {
    if(!c_options.jit)          // -nojit
        return;

#ifdef SHARP_JIT_X64
    jitCompiler = new X64Compiler();
#else
    // no compiler for this CPU: the interpreter runs everything
    c_options.jit = false;
#endif
}

void shutdown_jit_compiler() {
    if(jitCompiler != nullptr) {
        delete jitCompiler;
        jitCompiler = nullptr;
    }
}

bool should_compile_function(sharp_function* function) {
    // NOTE: do NOT test function->isHighFrequency here. prepare_method() only
    // sets that flag after compileFunction() succeeds (it means "compiled code is
    // installed and may be called"), so requiring it here would refuse every
    // compile and the jit would never run.
    return function != nullptr &&
           !function->nativeFunc &&
           function->bytecode != nullptr &&
           function->bytecodeSize > 0;
}
