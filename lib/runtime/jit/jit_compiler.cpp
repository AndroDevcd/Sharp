//
// JIT Compiler Base Implementation
//

#include "jit_compiler.h"
#include "arm/arm64_compiler.h"

JitCompiler* jitCompiler = nullptr;

void init_jit_compiler(jit_target_arch targetArch) {
    if(jitCompiler != nullptr) {
        shutdown_jit_compiler();
    }
    
    switch(targetArch) {
        case JIT_ARM64:
            jitCompiler = new Arm64Compiler();
            break;
        case JIT_X86_64:
            // TODO: Implement X86Compiler when needed
            throw std::runtime_error("X86_64 JIT compiler not yet implemented");
        case JIT_RISCV64:
            // TODO: Implement RISCV64Compiler when needed  
            throw std::runtime_error("RISC-V JIT compiler not yet implemented");
        default:
            throw std::runtime_error("Unsupported JIT target architecture");
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