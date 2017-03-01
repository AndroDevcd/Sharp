//
// Created by BraxtonN on 2/24/2017.
//

#include "FastStack.h"
#include "../oo/Exception.h"
#include "../internal/Environment.h"

void FastStack::free() {
    std::free (lst);
    lst = NULL;
    len = 0;
}

double FastStack::popInt() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    return lst[sp--].value;
}

void FastStack::push(double value) {
    sp++;
    if(sp >= len)
        throw Exception(Environment::StackOverflowErr, "");
    lst[sp].value = value;
}

void FastStack::pushs(string value) {
    sp++;
    if(sp >= len)
        throw Exception(Environment::StackOverflowErr, "");
    lst[sp].str = value;
}

void FastStack::pushs(nString value) {
    sp++;
    if(sp >= len)
        throw Exception(Environment::StackOverflowErr, "");
    lst[sp].str = value.str();
}

string FastStack::popString() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    return lst[sp--].str.str();
}

void FastStack::pusho(gc_object *value) {
    sp++;
    if(sp > len)
        throw Exception(Environment::StackOverflowErr, "");
    lst[sp].object.add(value);
}

void FastStack::popvoid() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    sp--;
}

gc_object *FastStack::popObject() {
    if(sp < 0)
        throw Exception(Environment::ThreadStackException, "illegal stack pop");
    gc_object* o = lst[sp].object.get();
    o->inv_reference(&lst[sp].object);
    lst[sp].object.notify();

    return o;
}

void FastStack::pushr(Reference *value) {
    sp++;
    if(sp > len)
        throw Exception(Environment::StackOverflowErr, "");
    lst[sp].object = *value;
}
