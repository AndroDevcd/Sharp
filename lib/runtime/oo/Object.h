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

enum sharp_type
{
    _stype_var = 0x1a,
    _stype_struct = 0x1b,
    _stype_none = 0x7f
};

struct SharpObject
{
    void init(int64_t size)
    {
        HEAD=NULL;
        k=NULL;
        next=NULL;
        prev=NULL;
//        mutex=NULL;
        this->size=size;
        type = _stype_none;
        refCount=0;
        gc_info = 0x000; /* generation young */
    }
    void init(int64_t size, ClassObject* k)
    {
        HEAD=NULL;
        next=NULL;
        prev=NULL;
        this->k=k;
//        mutex=NULL;
        this->size=size;
        type = _stype_none;
        refCount=0;
        gc_info = 0x000; /* generation young */
    }

    void print();

    union {
        double *HEAD;        /* data */
        Object *node;        /* structured data */
    };

    /* info */
    ClassObject* k;
    int64_t size;
    int32_t refCount;
    int8_t type;

    /**
     * collection generation
     *
     * layout
     * 0000 0000 8 bits consisting of "gc mark', 'generation', and 'lock'
     *
     *  000             0           0           000
     *  ^-- generation  ^-- mark    ^--- lock   ^--- unused
     */
    int8_t gc_info; /* gc stuff */
    SharpObject *next, *prev; /* linked list pointers */
};

#define DEC_REF(obj) \
    if(obj != NULL) { \
        obj->refCount--; \
        if(obj->refCount <= 0) { \
            switch(GENERATION((obj)->gc_info)) { \
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

    static void monitorLock(Object *o, Thread *t) {
        GarbageCollector::self->lock(o ? o->object : NULL, t);
    }

    static void monitorUnLock(Object *o, Thread *t) {
        GarbageCollector::self->unlock(o ? o->object : NULL, t);
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
