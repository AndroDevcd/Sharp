//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_GC_H
#define SHARP_GC_H

#include "../../../stdimports.h"
#include "../internal/Monitor.h"
#include "../oo/Object.h"

class GC {
public:
    static GC* gc;

    static void _init_GC();
    static void _collect_GC_CONCURRENT();
    static void _collect_GC_EXPLICIT();
    static void _insert(Sh_object*);
private:
    Monitor mutex;
    Sh_object** gc_alloc_heap;
    static void _collect();
};

void* memalloc(size_t bytes);
void* memcalloc(size_t bytes);

#endif //SHARP_GC_H
