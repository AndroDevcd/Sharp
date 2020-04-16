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
        currentFunction(NULL)
    {
        locals.init();
    }

    Scope(ClassObject* klass, BlockType bt)
            :
            type(bt),
            klass(klass),
            currentFunction(NULL)
    {
        locals.init();
    }


    Field* getLocalField(string name) {
        if(locals.size() == 0) return NULL;

        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i)->name == name) {
                return locals.get(i);
            }
        }
        return NULL;
    }

    List<Field*> locals;
    BlockType type;
    ClassObject* klass;
    Method* currentFunction;
};
// TODO: add Block *blck; to hold all nessicary block information
// add variable bool conStructorCalled; to be set if a constructor inside the class has been called
// in constructors if that boolean is still false at the end of the function then call decault consctuctor injected to the top of the function

#endif //SHARP_SCOPELEVEL_H
