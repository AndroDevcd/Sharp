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
#define default_stack 0x4ffc

class CallStack {

public:
    CallStack()
            :
            stack(NULL),
            len(0),
            sp(-1)
    {
    }

    void init() {
        stack = new Method*[default_stack];
        len = default_stack;
    }

    CXX11_INLINE
    void push(Method* method) {
        sp++;

        if(sp >= default_stack) throw Exception(""); // stack overflow error
        current = method;
        stack[sp] = method;
    }

    void pop() {
        sp--;

        if(sp < -1) {
            current = NULL;
            return;
        }

        current = stack[sp];
    }

    CXX11_INLINE
    Method* at(int32_t x) {
        if(x > len || x < 0)
            return NULL;
        return stack[x];
    }

    Method *current;
    int32_t len;

    void free() {
        if(len != 0) {
            std::free (stack); stack = NULL;
            current = NULL;
            sp = -1, len = 0;
        }
    }

private:
    int32_t sp;
    Method** stack;
};


#endif //SHARP_CALLSTACK_H
