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

struct s_it{
    double n;
    nString s;
    void* v;

    CXX11_INLINE
    void operator=(const s_it &s){
        n = s.n;
        this->s = s.s;
        v = s.v;
    }
};

class FastStack {
public:
    FastStack()
    :
            stack(NULL),
            len(0),
            sp(-1)
    {
    }

    void init() {
        stack = new s_it[default_stack];
        len = default_stack;
    }

    int32_t len;

    void push(gc_object* value);
    void push(nString value);
    void push(double value);
    gc_object* pop();
    nString pops();
    double popn();
    void cast32();
    void cast16();
    void cast64();
    void cast8();
    void castbool();
    void castfloat();

    void free();

    void popvoid();

    void swap();

private:
    s_it* stack;
    int32_t sp;

};


#endif //SHARP_FASTSTACK_H
