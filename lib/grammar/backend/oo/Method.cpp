//
// Created by BraxtonN on 10/22/2019.
//

#include "Method.h"
#include "../Compiler.h"

void Method::free() {
    release();
    Compiler::freeListPtr(params);
}