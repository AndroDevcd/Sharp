//
// Created by BraxtonN on 2/28/2017.
//

#include "CallStack.h"
#include "../oo/Method.h"

void CallStack::push(Method *method) {
    sp++;

    if(sp >= default_cstack) throw Exception(Environment::StackOverflowErr, ""); // stack overflow error
    current = method;
    stack[sp] = method;
    if(current->locals == 0)
        lstack[sp] = NULL;
    else
        lstack[sp] = new gc_object[current->locals];
    locals = lstack[sp];
}

void CallStack::pop() {
    sp--;

    if(sp <= -1) {
        Environment::free(locals, current->locals);
        current = NULL;
        return;
    }

    Environment::free(locals, current->locals);
    current = stack[sp];
    locals = lstack[sp];
}
