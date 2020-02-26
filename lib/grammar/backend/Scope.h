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

struct Local {
    long address;
    Field* field;
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


    Local* getLocalField(string name) {
        if(locals.size() == 0) return NULL;

        for(long long i = locals.size()-1; i >= 0; i--) {
            if(locals.at(i).field->name == name) {
                return &locals.get(i);
            }
        }
        return NULL;
    }

    List<Local> locals;
    BlockType type;
    ClassObject* klass;
    Method* currentFunction;
};

#endif //SHARP_SCOPELEVEL_H
