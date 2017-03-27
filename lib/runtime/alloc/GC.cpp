//
// Created by BraxtonN on 2/17/2017.
//

#include "GC.h"
#include "../internal/Thread.h"

size_t gc_max_heap_size = 640 * 1024 ;

void* memalloc(size_t bytes) {
    void* ptr =NULL;
    bool gc=false;
    alloc_bytes:
        ptr=malloc(bytes);

        if(ptr == NULL) {
            if(gc) {
                throw Exception("out of memory");
            } else {
                gc=true;
                GC::_collect_GC_EXPLICIT();
                goto alloc_bytes;
            }
        } else {
            return ptr;
        }
}

void GC::_collect_GC_CONCURRENT() {
    gc->mutex.acquire(INFINITE);
        _collect();
    gc->mutex.unlock();
}

void GC::_collect_GC_EXPLICIT() {
    gc->mutex.acquire(INFINITE);
    Thread::suspendAllThreads();
        _collect();
    Thread::releaseAllThreads();
    gc->mutex.unlock();
}

void GC::_collect() {
    for(unsigned int i=0; i<gc_max_heap_size; i++)
    {
        if(gc->gc_alloc_heap[i] != NULL) {
            gc->gc_alloc_heap[i]->free();
            std::free(gc->gc_alloc_heap[i]); gc->gc_alloc_heap[i] = NULL;
        }
    }
}

void GC::_init_GC() {
    gc=(GC*)memalloc(sizeof(GC)*1);
    gc->mutex = Monitor();
    gc->gc_alloc_heap=(Sh_object**)memcalloc(sizeof(Sh_object*)*gc_max_heap_size);
}

void GC::_insert(Sh_object *) {

}
