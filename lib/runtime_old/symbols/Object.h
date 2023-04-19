//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#ifndef WIN32_
#include <mutex>

#endif

#include <atomic>
#include "../memory/GarbageCollector.h"
#include "../symbols/ClassObject.h"

struct Object;
struct ClassObject;

enum sharp_type
{
    _stype_var = 0x1,
    _stype_struct = 0x2,
    _stype_none = 0x4
};

extern recursive_mutex refLock;
#define NTYPE_VAR 8

struct SharpObject
{
    void init(uInt size, sharp_type type, CollectionGeneration generation = gc_young)
    {
        info=0;
        ntype=0;
        HEAD=NULL;
        this->size=size;
        SET_INFO(info, 0, type, generation); /* generation young */
        refCount=0;
        refCount=0;
        monitor=0;
        array=0;
        next=NULL;
    }
    void init(uInt size, ClassObject* k, CollectionGeneration generation = gc_young)
    {
        info=0;
        ntype=0;
        HEAD=NULL;
        this->size=size;
        refCount=0;
        refCount=0;
        SET_INFO(info, k->address, _stype_struct, generation); /* generation young */
        SET_CLASS_BIT(info, 1);
        monitor=0;
        array=0;
        next=NULL;
    }

    void print();

    union {
        long double *HEAD;   /* data */
        Object *node;        /* structured data */
    };

    /* info */
    uint32_t size;
    uint32_t refCount;
    unsigned short monitor : 1; // used for the wait and notify() system
    unsigned short ntype : 4; // the type of number this object represents {var, _int32, _int64, etc}
    unsigned short array : 1; // wether or not this object is an array of data

    /**
     * Information package
     *
     * This variable represent all of the crucial data for the object that is mainly used by the garbage collector
     * as well as the JIT runtime_old.
     *
     * It is important to understand that this type of information is the only type of data that can be packed into this
     * integer. The data should be treated as "read-only" data as the garbage collector should be the only thread to manipulate
     * and modify the contents of the information. Therefore allowing no mutex requirement to be involved when acting on the
     * packaged information.
     *
     * layout
     * 00000000 00000000 00000000 00000000 32 bits consisting of "class", "type", "gc mark', 'generation', and 'lock'
     *
     *  low-end bits                                                                high-end bits
     *  00000000 00000000 00000000      000        0                00              0           0
     *  ^-- class address               ^--type    ^-- class bit    ^-- generation  ^-- mark    ^--- lock
     */
    uint32_t info;
    SharpObject *next; /* linked list pointers */
};

#define DEC_REF(obj) \
    if(obj != NULL) { \
        refLock.lock(); \
        obj->refCount--; \
        refLock.unlock(); \
    }

#define INC_REF(object) \
    if(object != NULL) { \
        refLock.lock(); \
        object->refCount++; \
        refLock.unlock(); \
    }

/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

    void wait();
    void notify();
    void notify(uInt);

    static bool monitorLock(Object *o, Thread *t) {
        return gc.lock(o ? o->object : NULL, t);
    }

    static void monitorUnLock(Object *o) {
        gc.unlock(o ? o->object : NULL);
    }

    CXX11_INLINE void operator=(Object &o) {
        if(&o == this || o.object==object) return;
        DEC_REF(this->object);

        this->object = o.object;
        INC_REF(object);
    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this || (o != NULL && o->object==object)) return;
        DEC_REF(this->object);

        this->object = o ? o->object : NULL;
        INC_REF(object);
    }
    CXX11_INLINE void operator=(SharpObject *o) {
        if(o == this->object) return;
        DEC_REF(this->object);

        this->object = o;
        INC_REF(object);
    }
    void castObject(int64_t classPtr);
};


#endif //SHARP_OBJECT_H
