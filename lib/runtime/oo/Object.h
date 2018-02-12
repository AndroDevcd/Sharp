//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_OBJECT_H
#define SHARP_OBJECT_H

#include "../Mutex.h"

struct Object;
struct ClassObject;

struct SharpObject
{
    void init()
    {
        HEAD=NULL;
        node=NULL;
        k=NULL;
        size=0;
        refCount=0;
        mutex=Mutex();
        generation = 0x000; /* generation young */
    }
    double *HEAD;        /* data */
    SharpObject *node;   /* data */

    /* info */
    ClassObject* k;
    unsigned long size;
    unsigned int refCount : 32;
    Mutex mutex;
    unsigned int generation : 3; /* collection generation */
};

/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
struct Object {
    SharpObject* object;

};


#endif //SHARP_OBJECT_H
