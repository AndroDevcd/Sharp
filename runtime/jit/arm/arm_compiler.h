//
// THE ARM JIT: the JitCompiler the VM talks to on ARM processors (jit_compiler.cpp picks it).
//
// It is the door only, as jit/x64/x64_compiler.h is for a PC: it drives one ArmGenerator per
// function (arm_generator.h has the map of the files) and installs the result - the executable
// memory and the jump table.
//
//      64-bit ARM (AArch64)    arm64/      machine code written with the AsmJit library
//      32-bit ARM              arm32/      machine code written by arm32/ itself
//
// What a compiled function looks like to the rest of the VM is exactly what jit/x64/x64_compiler.h
// describes: the same frames, the same jump table, the same return codes.
//
#pragma once

#include "../jit_compiler.h"

#if defined(SHARP_JIT_ARM64) || defined(SHARP_JIT_ARM32)

class ArmCompiler : public JitCompiler {
public:
    bool compileFunction(sharp_function* function, jit_compiled_function* output) override;
    void releaseCompiledFunction(jit_compiled_function* compiledFunc) override;

#ifdef SHARP_JIT_ARM64
private:
    asmjit::JitRuntime runtime;     // owns the executable memory
#endif
};

#endif
