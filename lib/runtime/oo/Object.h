//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include "../Mutex.h"
#include "../memory/GarbageCollector.h"
#include "../../util/mingw.mutex.h"

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
    recursive_mutex mutex;
    unsigned int _gcInfo : 3; /* collection generation */
};

/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

    CXX11_INLINE void operator=(Object &object) {
        if(&object == this) return;
        GarbageCollector::self->freeObject(this);

        this->object->refCount++;
        this->object = object.object;


    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this) return;
        GarbageCollector::self->freeObject(this);

        if(object != NULL)
        {
            this->object->refCount++;
            this->object = o->object;
        }
    }
    CXX11_INLINE void operator=(SharpObject *object) {
        if(object == this->object) return;
        GarbageCollector::self->freeObject(this);

        this->object = object;
    }
    void castObject(uint64_t classPtr);
};


#endif //SHARP_OBJECT_H
