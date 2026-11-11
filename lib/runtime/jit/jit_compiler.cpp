//
// JIT Compiler Base Implementation
//

#include "jit_compiler.h"

#include "arm/arm64_compiler.h"
#include "x64/x64_compiler.h"
#include "../../util/jit/asmjit/src/asmjit/core.h"
#include "../main.h"

JitCompiler* jitCompiler = nullptr;
jit_target_arch arch;
recursive_mutex jit_mutex;

void init_jit_compiler() {
    if(jitCompiler != nullptr) {
        shutdown_jit_compiler();
    }
    
    // Detect host architecture automatically
    Environment hostEnv = Environment::host();
    
    switch(hostEnv.arch()) {
        case Arch::kAArch64:
            arch = JIT_ARM64;
            jitCompiler = new Arm64Compiler();
            // The ARM64 backend does VM register / numeric array math with 64-bit
            // double instructions. That is only valid where long double == double
            // (Apple arm64). On Linux arm64 long double is a 128-bit type, so the
            // generated code would read garbage: keep the interpreter there.
            if(sizeof(long double) != sizeof(double)) {
                delete jitCompiler;
                jitCompiler = nullptr;
                c_options.jit = false;
            }
            break;
        case Arch::kX64:
            arch = JIT_X86_64;
            jitCompiler = new X64Compiler();
            break;
        case Arch::kX86:
            // 32-bit x86 is a different backend (different registers, calling
            // convention and pointer size). jit/x64 is 64-bit ONLY; a 32-bit
            // backend would live in its own jit/x86 folder. Until that exists
            // 32-bit hosts run in the interpreter.
            arch = JIT_X86_64;
            jitCompiler = nullptr; // todo: 32-bit x86 JIT compiler not yet implemented
            c_options.jit = false; // disable jit compilation
            break;
        default:
            throw std::runtime_error("Unsupported host architecture for JIT compilation");
    }
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