//
// Created by bknun on 9/18/2022.
//

#include "sharp_function.h"

void init_struct(sharp_function *sf) {
    sf->address = 0;
    sf->guid = 0;
    new (&sf->name) string();
    new (&sf->fullName)  string();
    new (&sf->lineTable)  list<line_data>();
    new (&sf->tryCatchTable)  list<try_catch_data>();
    sf->owner = nullptr;
    sf->flags = 0;
    sf->bytecode = nullptr;
    sf->fnType = normal_function;
    sf->stackSize = 0;
    sf->paramSize = 0;
    sf->returnType = nullptr;
    sf->arrayReturn = 0;
    sf->sourceFile = 0;
    sf->bytecodeSize = 0;
    sf->delegateAddress = 0;
    sf->spOffset = 0;
    sf->fpOffset = 0;
    sf->frameStackOffset = 0;
    sf->nativeFunc = false;
    sf->linkAddr = 0;
    sf->bridge = nullptr;
}
