//
// Created by BraxtonN on 2/24/2017.
//

#include "FastStack.h"
#include "../oo/Exception.h"
#include "../internal/Environment.h"

void FastStack::free() {
    std::free (stack);
    stack = NULL;
    len = 0;
}

void FastStack::push(gc_object *value) {
    if(++sp > len)
        throw Exception(Environment::StackOverflowErr, "");
    stack[sp].v = value;
}

void FastStack::popvoid() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    sp--;
}

void FastStack::swap() {
    if(sp < 1)
        throw Exception(Environment::ThreadStackException, "illegal stack swap");

    s_it pVoid = stack[sp];
    stack[sp] = stack[sp-1];
    stack[sp-1] = pVoid;
}

gc_object *FastStack::pop() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    return (gc_object*)stack[sp--].v;
}

void FastStack::push(nString value) {
    if(++sp >= len)
        throw Exception(Environment::StackOverflowErr, "");
    stack[sp].s = value;
}

nString FastStack::pops() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    return stack[sp--].s;
}

void FastStack::push(double value) {
    if(++sp >= len)
        throw Exception(Environment::StackOverflowErr, "");
    stack[sp].n = value;
}
double FastStack::popn() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    return stack[sp--].n;
}

void FastStack::cast32() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    stack[sp].n = (int32_t)stack[sp].n;
}

void FastStack::cast16() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    stack[sp].n = (int16_t)stack[sp].n;
}

void FastStack::cast64() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    stack[sp].n = (int64_t)stack[sp].n;
}

void FastStack::cast8() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    stack[sp].n = (int8_t)stack[sp].n;
}

void FastStack::castbool() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    stack[sp].n = (bool)stack[sp].n;
}

void FastStack::castfloat() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    stack[sp].n = (float)stack[sp].n;
}
