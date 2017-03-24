//
// Created by BraxtonN on 2/24/2017.
//

#ifndef SHARP_FASTSTACK_H
#define SHARP_FASTSTACK_H

#include "../../../stdimports.h"
#include "../oo/string.h"

#define default_stack 0xffe

class Sh_object;

struct s_it{
    double n;
    void* v;
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
        stack = (s_it*)malloc(sizeof(s_it)*default_stack);
        len = default_stack;
        sp=-1;
    }

    int32_t len;

    CXX11_INLINE
    void push(Sh_object* value){
        stack[++sp].v = value;
    }
    CXX11_INLINE
    void push(double value){
        stack[++sp].n = value;
    }

    CXX11_INLINE
    Sh_object* pop(){
        return (Sh_object*)stack[sp--].v;
    }

    CXX11_INLINE
    double popn(){
        return stack[sp--].n;
    }

    void free();

    void popvoid();

    void swap();

private:
    s_it* stack;
    int32_t sp;

};


#endif //SHARP_FASTSTACK_H
