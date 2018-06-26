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
    unsigned int generation : 3; /* collection generation 00 gen 0 mark */
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
        obj = nullptr; \
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
        if(&o == this) return;
        if(object!= NULL && object->size==1&&object->HEAD!= NULL) {
            int i = 0;
        }
        if(object != NULL)
        DEC_REF(this->object)

        if(o.object != NULL) {
            this->object = o.object;
            this->object->refCount++;
        }

    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this) return;
        if(object!= NULL && object->size==1&&object->HEAD!= NULL) {
            int i = 0;
        }
        DEC_REF(this->object)

        if(o->object != NULL)
        {
            this->object = o->object;
            this->object->refCount++;
        }
    }
    CXX11_INLINE void operator=(SharpObject *o) {
        if(o == this->object) return;
        if(object!= NULL && object->size==1&&object->HEAD!= NULL) {
            int i = 0;
        }
        DEC_REF(this->object)

        if(o != NULL)
        {
            this->object = o;
            this->object->refCount++;
        }
    }
    void castObject(uint64_t classPtr);

    void free();
};


#endif //SHARP_OBJECT_H
