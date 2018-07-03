//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#ifndef WIN32_
#include <mutex>
#endif

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
        next=NULL;
        prev=NULL;
        tail=NULL;
#ifdef WIN32_
        mutex.initalize();
#endif
#ifdef POSIX_
        new (&mutex) std::mutex();
#endif
        this->size=size;
        refCount=0;
        generation = 0x000; /* generation young */
    }
    void init(unsigned long size, ClassObject* k)
    {
        HEAD=NULL;
        node=NULL;
        next=NULL;
        prev=NULL;
        tail=NULL;
        this->k=k;
#ifdef WIN32_
        mutex.initalize();
#endif
#ifdef POSIX_
        new (&mutex) std::mutex();
#endif
        this->size=size;
        refCount=0;
        generation = 0x000; /* generation young */
    }

    void print();

    double *HEAD;        /* data */
    Object *node;        /* structured data */

    /* info */
    ClassObject* k;
    unsigned long size;
    long int refCount : 32;
#ifdef WIN32_
    recursive_mutex mutex;
#endif
#ifdef POSIX_
    std::mutex mutex;
#endif
    unsigned int generation : 4; /* collection generation 00 gen 0 mark */
    SharpObject *next, *prev, *tail; /* pointers for gc to use */
};

#define DEC_REF(obj) \
    if(obj != NULL) { \
        obj->refCount--; \
        switch(GENERATION((obj)->generation)) { \
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
    }

#define INC_REF(object) \
    if(object != NULL) { \
        object->refCount++; \
    }

/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

    CXX11_INLINE void operator=(Object &o) {
        if(&o == this || o.object==object) return;
        DEC_REF(this->object);

        if(o.object != NULL) {
            this->object = o.object;
            this->object->refCount++;
        } else object=NULL;
    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this || (o != NULL && o->object==object)) return;
        DEC_REF(this->object);

        if(o != NULL && o->object != NULL)
        {
            this->object = o->object;
            this->object->refCount++;
        } else object=NULL;
    }
    CXX11_INLINE void operator=(SharpObject *o) {
        if(o == this->object) return;
        DEC_REF(this->object);

        this->object = o;
        if(o != NULL) {
            o->refCount++;
        }
    }
    void castObject(uint64_t classPtr);
};


#endif //SHARP_OBJECT_H
