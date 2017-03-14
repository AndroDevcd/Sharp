//
// Created by BraxtonN on 2/28/2017.
//

#ifndef SHARP_CALLSTACK_H
#define SHARP_CALLSTACK_H

#include "../../../stdimports.h"
#include "../oo/Exception.h"
#include "../internal/Environment.h"

class Method;

/* 20k recursive calls */
#define default_cstack 0x4ffc

class CallStack {

public:
    CallStack()
            :
            stack(NULL),
            locals(NULL),
            len(0),
            sp(-1)
    {
    }

    void init() {
        stack = new Method*[default_cstack];
        lstack = new gc_object*[default_cstack];

        for(long i = 0; i < default_cstack; i++) {
            lstack[i] = NULL;
        }
        len = default_cstack;
    }

    void push(Method* method);

    void pop();

    CXX11_INLINE
    Method* at(int32_t x) {
        if(x > len || x < 0)
            return NULL;
        return stack[x];
    }


    void Execute();

    gc_object* instance;
    gc_object* locals;
    Method *current;
    int32_t len;

    void free() {
        if(len != 0) {
            std::free (stack); stack = NULL;
            std::free (lstack); lstack = NULL;
            current = NULL;
            sp = -1, len = 0;
        }
    }

private:
    int32_t sp;
    Method** stack;
    gc_object** lstack;
};


#endif //SHARP_CALLSTACK_H
