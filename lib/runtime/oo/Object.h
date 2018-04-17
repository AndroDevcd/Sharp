//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include "../Mutex.h"
#include "../memory/GarbageCollector.h"

struct Object;
struct ClassObject;

struct SharpObject
{
    void init()
    {
        HEAD=NULL;
        node=NULL;
        k=NULL;
        mutex.initalize();
        size=0;
        refCount=0;
        _gcInfo = 0x000; /* generation young */
    }
    double *HEAD;        /* data */
    Object *node;        /* structured data */

    /* info */
    ClassObject* k;
    unsigned long size;
    unsigned int refCount : 32;
#ifdef WIN32_
    recursive_mutex mutex;
#endif
#ifdef POSIX
    std::mutex mutex;
#endif
    unsigned int _gcInfo : 3; /* collection generation */
};

#define FREE_OBJ \
    std::lock_guard<recursive_mutex> guard(mutex); \
    MARK_FOR_DELETE(object->_gcInfo, 1); \
    object->refCount--; \
     \
    switch(GENERATION(object->_gcInfo)) { \
        case gc_young: \
            GarbageCollector::self->yObjs++; \
            break; \
        case gc_adult: \
            GarbageCollector::self->aObjs++; \
            break; \
        case gc_old: \
            GarbageCollector::self->oObjs++; \
            break; \
    } \
    object = nullptr;

/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

    CXX11_INLINE void operator=(Object &o) {
        if(&o == this) return;
        FREE_OBJ

        this->object->refCount++;
        this->object = o.object;


    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this) return;
        FREE_OBJ

        if(object != NULL)
        {
            this->object->refCount++;
            this->object = o->object;
        }
    }
    CXX11_INLINE void operator=(SharpObject *object) {
        if(object == this->object) return;
        FREE_OBJ

        this->object = object;
    }
    void castObject(uint64_t classPtr);
};


#endif //SHARP_OBJECT_H
