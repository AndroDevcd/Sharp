//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_GARBAGECOLLECTOR_H
#define SHARP_GARBAGECOLLECTOR_H

#include "../Mutex.h"

enum CollectionPolicy
{
    GC_LOW,
    GC_CONCURRENT,
    GC_EXPLICIT
};

class Object;
struct SharpObject;

class GarbageCollector {
public:
    static GarbageCollector *self;
    CollectionPolicy policy;
    Mutex mutex, threadMutex;

    void initilize();
    void shutdown();
    void start();

    /**
     * @Policy: GC_LOW
     * The garbage collector will stop all threads to deallocate memory
     * this will only happen if the physical memory on the computer is running low
     * during allocation
     *
     * @Policy: GC_EXPLICIT
     * Explicit collection always will happen on the thread to which its called upon
     *
     * @Policy: GC_CONCURRENT
     * Concurrent collections will occur as the GC
     */
    void collect();

    Object* newObject(unsigned long size); /* Array allocation */
    Object* newObject(unsigned long size, ClassObject* k); /* Class allocation */

private:
    unsigned long managedBytes;
    unsigned long memoryLimit;
    List<SharpObject*> heap;

    void freeObject(Object* object);

};


#endif //SHARP_GARBAGECOLLECTOR_H
