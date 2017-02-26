//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_FASTSTACK_H
#define SHARP_FASTSTACK_H

#include "../../../stdimports.h"

#define default_stack 0xffe

class gc_object;

struct StackItem {
    double value;
    gc_object* object;
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
        lst = (StackItem*) malloc(default_stack*sizeof(StackItem));
        len = default_stack;
    }

    int32_t len;

    int popInt();

    void free();

private:
    StackItem* lst;
    int32_t sp;
};


#endif //SHARP_FASTSTACK_H
