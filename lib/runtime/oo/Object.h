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
    void init()
    {
        data=NULL;
        node=NULL;
        k=NULL;
#ifdef WIN32_
        mutex.initalize();
#endif
#ifdef POSIX_
        mutex = std::mutex();
#endif
        size=0;
        refCount=0;
        mark = 0x000; /* generation young */
    }
    double *data;        /* data */
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
    unsigned int mark : 1; /* collection generation */
};


/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

    CXX11_INLINE static void newRefrence(Object *object, SharpObject *o) {
        if (object->object==o) return;
        if(object->object != nullptr)
        {
            object->object->refCount--;
            GarbageCollector::self->objs++;
            object->object = nullptr;
        }

        object->object = o;
    }

    CXX11_INLINE void operator=(Object &o) {
        if(&o == this) return;
        if(object != nullptr)
        {
            object->refCount--;
            GarbageCollector::self->objs++;
            object = nullptr;
        }

        if(o.object != NULL) {
            this->object = o.object;
            this->object->refCount++;
        }

    }
    CXX11_INLINE void operator=(Object *o) {
        if(o == this) return;
        if(object != nullptr)
        {
            object->refCount--;
            GarbageCollector::self->objs++;
            object = nullptr;
        }

        if(o->object != NULL)
        {
            this->object = o->object;
            this->object->refCount++;
        }
    }
    CXX11_INLINE void operator=(SharpObject *o) {
        if(o == this->object) return;
        if(object != nullptr)
        {
            object->refCount--;
            GarbageCollector::self->objs++;
            object = nullptr;
        }

        this->object = o;
    }
    void castObject(uint64_t classPtr);
};


#endif //SHARP_OBJECT_H
