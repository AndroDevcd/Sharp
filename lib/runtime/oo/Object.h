//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include "../memory/GarbageCollector.h"

struct Object;
struct ClassObject;

struct SharpObject
{
    void init(unsigned long size)
    {
        HEAD=NULL;
        node=NULL;
        k=NULL;
#ifdef WIN32_
        mutex.initalize();
#endif
#ifdef POSIX_
        mutex = std::mutex();
#endif
        this->size=size;
        refCount=1;
        generation = 0x000; /* generation young */
    }
    void init(unsigned long size, ClassObject* k)
    {
        HEAD=NULL;
        node=NULL;
        this->k=k;
#ifdef WIN32_
        mutex.initalize();
#endif
#ifdef POSIX_
        mutex = std::mutex();
#endif
        this->size=size;
        refCount=1;
        generation = 0x000; /* generation young */
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
#ifdef POSIX_
    std::mutex mutex;
#endif
    unsigned int generation : 3; /* collection generation */
};

#define DEC_REF \
    if(object != NULL) { \
        object->refCount--; \
         \
        switch(object->generation) { \
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
        object = nullptr; \
    }


/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

    CXX11_INLINE void operator=(Object &o) {
        if(&o == this) return;
        DEC_REF

        if(o.object != NULL) {
            this->object = o.object;
            this->object->refCount++;
        }

    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this) return;
        DEC_REF

        if(o->object != NULL)
        {
            this->object = o->object;
            this->object->refCount++;
        }
    }
    CXX11_INLINE void operator=(SharpObject *o) {
        if(o == this->object) return;
        DEC_REF

        this->object = o;
    }
    void castObject(uint64_t classPtr);
};


#endif //SHARP_OBJECT_H
