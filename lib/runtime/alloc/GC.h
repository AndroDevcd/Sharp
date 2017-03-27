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

    static void GCStartup();
    static void GCShutdown();
    static void _init_GC();
    static void _collect_GC_CONCURRENT();
    static void _collect_GC_EXPLICIT();
    static void _insert(Sh_object*);
    static void _insert_stack(Sh_object*, unsigned long);
private:
    Monitor mutex;
    Sh_object* gc_alloc_heap;
    unsigned long allocptr;
    static void _collect();

    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
     _GCThread_start(void *);

    void _GC_run();
};

#define _GC_CAP_THRESHOLD 1.5

#endif //SHARP_GC_H
