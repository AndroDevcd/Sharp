//
// Created by Dev {Code} on 8/21/25.
//
#include "../arm64_compiler.h"

bool Arm64Compiler::emit_nop() {
    assembler->nop();
    return true;
}