//
// Created by BraxtonN on 2/17/2017.
//

#include "GC.h"
#include "../internal/Thread.h"
#include "../../../stdimports.h"

size_t gc_max_heap_size = 640 * 1024 ;
GC* GC::gc = NULL;

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

void* memcalloc(size_t count, size_t bytes) {
    void* ptr =NULL;
    bool gc=false;
    alloc_bytes:
        ptr=calloc(count, bytes);

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

void* memrealloc(void* Ptr, size_t bytes) {
    void* ptr =NULL;
    bool gc=false;
    alloc_bytes:
    ptr=realloc(Ptr, bytes);

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
    Thread::resumeAllThreads();
    gc->mutex.unlock();
}

void GC::_collect() {
    for(unsigned int i=0; i<gc->allocptr; i++)
    {
        gc->gc_alloc_heap[i].free();
    }
    gc->allocptr=0;
}

void GC::_init_GC() {
    gc=(GC*)malloc(sizeof(GC)*1);
    gc->mutex = Monitor();
    gc->gc_alloc_heap=(Sh_object*)malloc(sizeof(Sh_object)*gc_max_heap_size);
    Environment::init(gc->gc_alloc_heap, gc_max_heap_size);
    gc->allocptr=0;
}

void GC::_insert(Sh_object *gc_obj) {
    gc->mutex.acquire(INFINITE);
    if(gc->allocptr == gc_max_heap_size) {
        _collect_GC_EXPLICIT();
    }

    gc->gc_alloc_heap[gc->allocptr].nxt=gc_obj->nxt;
    gc->gc_alloc_heap[gc->allocptr].prev=gc_obj->prev;
    gc->gc_alloc_heap[gc->allocptr].HEAD=gc_obj->HEAD;
    gc->gc_alloc_heap[gc->allocptr]._Node=gc_obj->_Node;
    gc->gc_alloc_heap[gc->allocptr].size=gc_obj->size;
    gc->allocptr++;
    gc->mutex.unlock();
}

void GC::GCStartup() {
    _init_GC();

    Thread* gcThread=(Thread*)malloc(
            sizeof(Thread)*1);
    gcThread->CreateDaemon("gc");
    Thread::startDaemon(
            GC::_GCThread_start, gcThread);
}

void GC::GCShutdown() {
    if(gc != NULL) {
        if(gc->gc_alloc_heap != NULL) {
            std::free(gc->gc_alloc_heap);
            gc->gc_alloc_heap=NULL;
        }
        gc->allocptr=0;

        std::free(gc);
        gc=NULL;
    }
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
 GC::_GCThread_start(void *pVoid) {
    thread_self =(Thread*)pVoid;
    thread_self->state = thread_running;

    gc->_GC_run();

    /*
         * Check for uncaught exception in thread before exit
         */
    thread_self->exit();

#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return NULL;
#endif
}

void GC::_GC_run() {
    const unsigned int sMaxRetries = 10000 * 1000;
    unsigned int retryCount = 0;

    for(;;) {
        if(thread_self->suspendPending) {
            Thread::suspendSelf();
        }
        if(thread_self->state == thread_killed) {
            return;
        }

        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
#ifdef WIN32_
            Sleep(2);
#endif
#ifdef POSIX_
            usleep(2*POSIX_USEC_INTERVAL);
#endif
        } else {
            if(gc->allocptr>(gc_max_heap_size/_GC_CAP_THRESHOLD)){
                gc->mutex.acquire(INFINITE);
                GC::_collect_GC_CONCURRENT();
                gc->mutex.unlock();
            }
        }

    }
}
