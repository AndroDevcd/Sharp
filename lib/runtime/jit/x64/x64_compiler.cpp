//
// The x86-64 JitCompiler: what the VM calls (jit_try_compile in
// virtual_machine.cpp). It drives one X64Generator per function and installs
// the result. The machine code itself is written by the generator - see
// x64_generator.h for the map of the files.
//

#include "x64_compiler.h"
#include "x64_generator.h"
#include "../../telemetry/telemetry.h"

using namespace asmjit;
using namespace x64jit;

bool X64Compiler::compileFunction(sharp_function* function, jit_compiled_function* output) {
    if(!should_compile_function(function))
        return false;

    CodeHolder holder;
    holder.init(runtime.environment(), runtime.cpuFeatures());
    JitErrorHandler errors;
    holder.setErrorHandler(&errors);
    x86::Assembler assembler(&holder);

    X64Generator generator(assembler, function, output);
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

    // The jump table: the machine code address of every instruction. Words that
    // are not the start of one (the second word of a two word instruction) lead
    // to the "decline" exit.
    output->jumpTable = (void**)calloc((size_t)generator.count + 1, sizeof(void*));
    if(output->jumpTable == nullptr) {
        runtime.release(entry);
        return false;
    }

    const uint8_t *base = reinterpret_cast<const uint8_t*>(entry);
    void *declineAddress = (void*)(base + holder.labelOffsetFromBase(generator.declineFromRegisters));
    for(Int pc = 0; pc < generator.count; pc++) {
        output->jumpTable[pc] = generator.starts[pc]
                ? (void*)(base + holder.labelOffsetFromBase(generator.labels[pc]))
                : declineAddress;
    }

    output->jumpTableSize = (size_t)generator.count;
    output->compiledCode = entry;
    output->directEntry = (void*)(base + holder.labelOffsetFromBase(generator.directEntry));
    output->codeSize = holder.codeSize();
    output->originalFunction = function;
    output->isCompiled = true;
    return true;
}

void X64Compiler::releaseCompiledFunction(jit_compiled_function* compiledFunc) {
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
