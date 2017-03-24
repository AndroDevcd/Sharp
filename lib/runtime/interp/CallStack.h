//
// Created by BraxtonN on 2/28/2017.
//

#ifndef SHARP_CALLSTACK_H
#define SHARP_CALLSTACK_H

#include "../../../stdimports.h"
#include "../oo/Exception.h"
#include "../internal/Environment.h"

class Method;

/* 8k recursive calls */
#define default_cstack 0x1fc0

struct gc_stack {
    Sh_object* locals;
    double rgs[10];
    Method* callee;
};

class CallStack {

public:
    CallStack()
            :
            stack(NULL),
            locals(NULL),
            instance(NULL),
            len(0),
            sp(-1)
    {
    }

    void init() {
        stack = (gc_stack*)malloc(sizeof(gc_stack)*default_cstack);
        len = default_cstack;
        sp=-1;
    }

    void push(Method* method);

    void pop();

    CXX11_INLINE
    Method* at(int32_t x) {
        if(x > len || x < 0)
            return NULL;
        return stack[x].callee;
    }


    void Execute();

    Sh_object* instance;
    Sh_object* locals;
    Method *current;
    double* regs;
    int32_t len;
    FastStack* thread_stack;

    void free() {
        if(len != 0) {
            std::free (stack); stack = NULL;
            current = NULL;
            sp = -1, len = 0;
        }
    }

private:
    int32_t sp;
    gc_stack* stack;
};


#endif //SHARP_CALLSTACK_H
