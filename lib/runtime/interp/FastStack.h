//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_FASTSTACK_H
#define SHARP_FASTSTACK_H

#include "../../../stdimports.h"
#include "../oo/Reference.h"
#include "../oo/string.h"

#define default_stack 0xffe

class gc_object;

struct StackItem {
    double value;
    Reference object;
    nString str;
};

class FastStack {
public:
    FastStack()
    :
            lst(NULL),
            len(0),
            sp(-1)
    {
    }

    void init() {
        lst = new StackItem[default_stack];
        len = default_stack;
    }

    int32_t len;

    void push(double value);
    void pushs(string value);
    void pushs(nString value);
    void pusho(gc_object* value);
    void pushr(Reference* value);
    double popInt();
    string popString();
    gc_object* popObject();

    void free();

    void popvoid();

private:
    StackItem* lst;
    int32_t sp;
};


#endif //SHARP_FASTSTACK_H
