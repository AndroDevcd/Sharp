//
// Created by BraxtonN on 2/24/2017.
//

#include "FastStack.h"
#include "../oo/Exception.h"
#include "../internal/Environment.h"

void FastStack::free() {
    if(stack != NULL) {
        std::free (stack);
        stack = NULL;
    }
    len = 0;
}

void FastStack::popvoid() {
    if(sp < 0)
        throw Exception(&Environment::ThreadStackException, "illegal stack pop");
    sp--;
}

void FastStack::swap() {
    if(sp < 1)
        throw Exception(&Environment::ThreadStackException, "illegal stack swap");

    s_it pVoid = stack[sp];
    stack[sp] = stack[sp-1];
    stack[sp-1] = pVoid;
}
