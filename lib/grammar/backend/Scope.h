//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_SCOPELEVEL_H
#define SHARP_SCOPELEVEL_H

#include "oo/Method.h"

enum BlockType {
    GLOBAL_SCOPE,
    CLASS_SCOPE,
    INSTANCE_BLOCK,
    STATIC_BLOCK
};

struct Scope {
    public:
    Scope()
    :
        type(GLOBAL_SCOPE),
        klass(NULL),
        currentFunction(NULL)
    {
    }

    Scope(ClassObject* klass, BlockType bt)
            :
            type(bt),
            klass(klass),
            currentFunction(NULL)
    {
    }

    BlockType type;
    ClassObject* klass;
    Method* currentFunction;
};

#endif //SHARP_SCOPELEVEL_H
