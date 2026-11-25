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
    dump_machine_code(function, base, holder.codeSize(), output);
    return true;
}

/**
 * FOR WORKING ON THE JIT: the machine code of one function, written to files.
 *
 *      SHARP_JIT_DUMP=advance sharp program        every compiled function whose full name has "advance" in it
 *
 * writes, into the current folder, `jit_<n>.bin` (the code, as it is in memory) and `jit_<n>.txt`
 * (the function's name, and for every bytecode instruction where its machine code begins).
 * Read the code with  objdump -D -b binary -m i386:x86-64 jit_<n>.bin
 */
void X64Compiler::dump_machine_code(sharp_function *function, const uint8_t *base, size_t size, jit_compiled_function *output) {
    static const char *wanted = getenv("SHARP_JIT_DUMP");
    if(wanted == nullptr || function->fullName.find(wanted) == string::npos) return;
    static int number = 0;
    string stem = "jit_" + to_string(number++);
    if(FILE *code = fopen((stem + ".bin").c_str(), "wb")) {
        fwrite(base, 1, size, code);
        fclose(code);
    }
    if(FILE *map = fopen((stem + ".txt").c_str(), "w")) {
        fprintf(map, "%s   (%zu bytes)\n", function->fullName.c_str(), size);
        for(size_t pc = 0; pc < output->jumpTableSize; pc++) {
            const uint8_t *at = reinterpret_cast<const uint8_t*>(output->jumpTable[pc]);
            if(at >= base && at < base + size) fprintf(map, "pc %zu at 0x%zx\n", pc, (size_t)(at - base));
        }
        fclose(map);
    }
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
