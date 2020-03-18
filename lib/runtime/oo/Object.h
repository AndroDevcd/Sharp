//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#ifndef WIN32_
#include <mutex>
#include <atomic>

#endif

#include "../memory/GarbageCollector.h"
#include "../oo/ClassObject.h"

struct Object;
struct ClassObject;

enum sharp_type
{
    _stype_var = 0x1,
    _stype_struct = 0x2,
    _stype_none = 0x4
};

struct SharpObject
{
    void init(uint32_t size)
    {
        info=0;
        HEAD=NULL;
        next=NULL;
        prev=NULL;
//        mutex=NULL;
        this->size=size;
        SET_INFO(info, 0, _stype_none, gc_young); /* generation young */
        refCount=0;
    }
    void init(uint32_t size, ClassObject* k)
    {
        info=0;
        HEAD=NULL;
        next=NULL;
        prev=NULL;
//        mutex=NULL;
        this->size=size;
        refCount=0;
        SET_INFO(info, k->serial, _stype_none, gc_young); /* generation young */
        SET_CLASS_BIT(info, 1);
    }

    void print();

    union {
        double *HEAD;        /* data */
        Object *node;        /* structured data */
    };

    /* info */
    uint32_t size;
    std::atomic<uint32_t> refCount;

    /**
     * Information package
     *
     * This variable represent all of the crucial data for the object that is mainly used by the garbage collector
     * as well as the JIT runtime.
     *
     * It is important to understand that this type of information is the only type of data that can be packed into this
     * integer. The data should be treated as "read-only" data as the garbage collector should be the only thread to manipulate
     * and modify the contents of the information. Therefore allowing no mutex requirement to be involved when acting on the
     * packaged information.
     *
     * layout
     * 0000 0000 0000 0000 0000 0000 0000 0000 32 bits consisting of "class", "type", "gc mark', 'generation', and 'lock'
     *
     *  low-end bits                                                                high-end bits
     *  0000 0000 0000 0000 0000 0000   000        0                00              0           0
     *  ^-- class address               ^--type    ^-- class bit    ^-- generation  ^-- mark    ^--- lock
     */
    uint32_t info;
    SharpObject *next, *prev; /* linked list pointers */
};

#define DEC_REF(obj) \
    if(obj != NULL) { \
        obj->refCount--; \
        if(obj->refCount <= 0) { \
            switch(GENERATION((obj)->info)) { \
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
