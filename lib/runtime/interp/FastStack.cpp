//
// Created by BraxtonN on 2/24/2017.
//

#include "FastStack.h"
#include "../oo/Exception.h"

void FastStack::free() {
    std::free (lst);
    lst = NULL;
    len = 0;
}

int FastStack::popInt() {
    if(sp < 0)
        throw Exception("Thread stack exception, illegal stack pop");
    return (int)lst[sp--].value;
}

void FastStack::push(double value) {
    sp++;
    if(sp > len)
        throw Exception("Thread stack exception, illegal stack pop");
    lst[sp].value = value;
}

void FastStack::pushs(string value) {
    sp++;
    if(sp > len)
        throw Exception("Thread stack exception, illegal stack pop");
    lst[sp].str = value;
}

string FastStack::popString() {
    if(sp < 0)
        throw Exception("Thread stack exception, illegal stack pop");
    return lst[sp--].str.str();
}
