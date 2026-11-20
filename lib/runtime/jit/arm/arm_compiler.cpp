//
// The ARM JitCompiler: what the VM calls (jit_try_compile in virtual_machine.cpp). It drives one
// ArmGenerator per function and installs the result. The machine code itself is written by the
// generator - see arm_generator.h for the map of the files.
//

#include "arm_compiler.h"

#ifdef SHARP_JIT_ARM64

#include "arm_generator.h"
#include "arm64/arm64_machine.h"
#include "../../telemetry/telemetry.h"

using namespace asmjit;
using namespace armjit;

bool ArmCompiler::compileFunction(sharp_function* function, jit_compiled_function* output) {
    if(!should_compile_function(function))
        return false;

    CodeHolder holder;
    holder.init(runtime.environment(), runtime.cpuFeatures());
    JitErrorHandler errors;
    holder.setErrorHandler(&errors);
    a64::Assembler assembler(&holder);
    Arm64Machine machine(assembler, errors);

    ArmGenerator generator(machine, function, output);
    if(!generator.generate()) {
        // an instruction the JIT does not handle: the function stays interpreted
        TELEMETRY(telemetry_jit_blocked(function, generator.blockedBy);)
        return false;
    }

    if(errors.failed)
        return false;

    // make the code executable
    jit_function_ptr entry = nullptr;
    if(runtime.add(&entry, &holder) != kErrorOk || entry == nullptr)
        return false;

    // The jump table: the machine code address of every instruction. Words that are not the start
    // of one (the second word of a two word instruction) lead to the "decline" exit.
    output->jumpTable = (void**)calloc((size_t)generator.count + 1, sizeof(void*));
    if(output->jumpTable == nullptr) {
        runtime.release(entry);
        return false;
    }

    const uint8_t *base = reinterpret_cast<const uint8_t*>(entry);
    void *declineAddress = (void*)(base + holder.labelOffsetFromBase(machine.place(generator.decline)));
    for(Int pc = 0; pc < generator.count; pc++) {
        output->jumpTable[pc] = generator.starts[pc]
                ? (void*)(base + holder.labelOffsetFromBase(machine.place(generator.labels[pc])))
                : declineAddress;
    }

    output->jumpTableSize = (size_t)generator.count;
    output->compiledCode = entry;
    output->directEntry = (void*)(base + holder.labelOffsetFromBase(machine.place(generator.directEntry)));
    output->codeSize = holder.codeSize();
    output->originalFunction = function;
    output->isCompiled = true;
    return true;
}

void ArmCompiler::releaseCompiledFunction(jit_compiled_function* compiledFunc) {
    if(compiledFunc == nullptr) return;

    if(compiledFunc->compiledCode != nullptr)
        runtime.release(compiledFunc->compiledCode);
    if(compiledFunc->jumpTable != nullptr)
        free(compiledFunc->jumpTable);

    compiledFunc->compiledCode = nullptr;
    compiledFunc->directEntry = nullptr;
    compiledFunc->jumpTable = nullptr;
    compiledFunc->isCompiled = false;
}

#endif // SHARP_JIT_ARM64

#ifdef SHARP_JIT_ARM32

#include "arm_generator.h"
#include "arm32/arm32_machine.h"
#include "../../telemetry/telemetry.h"

using namespace armjit;

bool ArmCompiler::compileFunction(sharp_function* function, jit_compiled_function* output) {
    if(!should_compile_function(function))
        return false;

    Arm32Machine machine;
    ArmGenerator generator(machine, function, output);
    if(!generator.generate()) {
        // an instruction the JIT does not handle: the function stays interpreted
        TELEMETRY(telemetry_jit_blocked(function, generator.blockedBy);)
        return false;
    }

    if(machine.failed())
        return false;

    // (the jump table first: once the code is in executable memory it is not given back)
    output->jumpTable = (void**)calloc((size_t)generator.count + 1, sizeof(void*));
    if(output->jumpTable == nullptr)
        return false;

    const uint8_t *base = reinterpret_cast<const uint8_t*>(machine.install());
    if(base == nullptr) {
        free(output->jumpTable);
        output->jumpTable = nullptr;
        return false;
    }

    // The jump table: the machine code address of every instruction. Words that are not the start
    // of one (the second word of a two word instruction) lead to the "decline" exit.
    void *declineAddress = (void*)(base + machine.offset_of(generator.decline));
    for(Int pc = 0; pc < generator.count; pc++) {
        output->jumpTable[pc] = generator.starts[pc]
                ? (void*)(base + machine.offset_of(generator.labels[pc]))
                : declineAddress;
    }

    output->jumpTableSize = (size_t)generator.count;
    output->compiledCode = reinterpret_cast<jit_function_ptr>(const_cast<uint8_t*>(base));     // the C++ door is the first instruction
    output->directEntry = (void*)(base + machine.offset_of(generator.directEntry));
    output->codeSize = machine.code_bytes();
    output->originalFunction = function;
    output->isCompiled = true;
    return true;
}

void ArmCompiler::releaseCompiledFunction(jit_compiled_function* compiledFunc) {
    if(compiledFunc == nullptr) return;

    // (the machine code stays where it is: arm32_machine.cpp, "memory the processor may run")
    if(compiledFunc->jumpTable != nullptr)
        free(compiledFunc->jumpTable);

    compiledFunc->compiledCode = nullptr;
    compiledFunc->directEntry = nullptr;
    compiledFunc->jumpTable = nullptr;
    compiledFunc->isCompiled = false;
}

#endif // SHARP_JIT_ARM32
