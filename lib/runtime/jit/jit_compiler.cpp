//
// JIT Compiler Base Implementation
//

#include "jit_compiler.h"

#include "arm/arm64_compiler.h"
#include "../../util/jit/asmjit/src/asmjit/core.h"
#include "../main.h"

JitCompiler* jitCompiler = nullptr;
jit_target_arch arch;

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
            break;
        case Arch::kX64:
            arch = JIT_X86_64;
            jitCompiler = nullptr; // todo: X86_64 JIT compiler not yet implemented
            c_options.jit = false; // disable jit compilation
            break;
        case Arch::kX86:
            arch = JIT_X86_64; // Treat 32-bit x86 as x86_64 target
            jitCompiler = nullptr; // todo: X86_64 JIT compiler not yet implemented
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
    return function != nullptr && 
           function->isHighFrequency && 
           !function->nativeFunc &&
           function->bytecode != nullptr &&
           function->bytecodeSize > 0;
}