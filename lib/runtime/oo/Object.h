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
        mutex=NULL;
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
        this->k=k;
        mutex=NULL;
        this->size=size;
        refCount=0;
        generation = 0x000; /* generation young */
    }

    void print();

    double *HEAD;        /* data */
    Object *node;        /* structured data */

    /* info */
    ClassObject* k;
    int64_t size;
    long int refCount : 32;
#ifdef WIN32_
    recursive_mutex* mutex;
#endif
#ifdef POSIX_
    recursive_mutex* mutex;
#endif
    /**
     * collection generation
     *
     * layout
     * 0000 4 bits consisting of "gc mark' and 'generation'
     *
     * 0            000
     * ^-- mark     ^-- generation
     */
    unsigned int generation : 4; /* gc stuff */
    SharpObject *next, *prev; /* linked list pointers */
};

#define DEC_REF(obj) \
    if(obj != NULL) { \
        obj->refCount--; \
        if(obj->refCount <= 0) { \
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
        }\
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

    void monitorLock();

    void monitorUnLock() {
        if(object && object->mutex != NULL) {
            object->mutex->unlock();
        }
    }

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
    void castObject(int64_t classPtr);
};


#endif //SHARP_OBJECT_H
