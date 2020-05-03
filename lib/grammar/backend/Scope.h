//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_SCOPELEVEL_H
#define SHARP_SCOPELEVEL_H

#include "oo/Method.h"

enum BlockType {
    GLOBAL_SCOPE=0,
    CLASS_SCOPE=1,
    RESTRICTED_INSTANCE_BLOCK=2,
    INSTANCE_BLOCK=3,
    STATIC_BLOCK=4
};

struct Scope {
    public:
    Scope()
    :
            type(GLOBAL_SCOPE),
            klass(NULL),
            currentFunction(NULL),
            isReachable(true),
            scopeLevel(0),
            loopStartLabel(""),
            loopEndLabel(""),
            statements(),
            finallyBlocks(),
            lockBlocks()
    {
    }

    Scope(ClassObject* klass, BlockType bt)
            :
            type(bt),
            klass(klass),
            currentFunction(NULL),
            isReachable(true),
            scopeLevel(0),
            loopStartLabel(""),
            loopEndLabel(""),
            statements(),
            finallyBlocks(),
            lockBlocks()
    {
    }

    void resetLocalScopeFlags() {
        currentFunction = NULL;
        scopeLevel = 0;
        isReachable = true;
        loopStartLabel = "";
        loopEndLabel = "";
        statements.free();
        finallyBlocks.free();
        lockBlocks.free();
    }

    BlockType type;
    ClassObject* klass;
    Method* currentFunction;
    Int scopeLevel;
    bool isReachable;
    string loopStartLabel;
    string loopEndLabel;
    List<ast_type> statements; // use this to figure out how many finally blocks we need to execute until we hit a loop ast or whatever we need and compile the respective block starting from the end of the list backwards
    List<Ast*> finallyBlocks; // as we parse try catches populate this if there is no finally block to process then set this to null
    List<Ast*> lockBlocks;
};
// TODO: add Block *blck; to hold all nessicary block information
// add variable bool conStructorCalled; to be set if a constructor inside the class has been called
// in constructors if that boolean is still false at the end of the function then call decault consctuctor injected to the top of the function

#endif //SHARP_SCOPELEVEL_H
