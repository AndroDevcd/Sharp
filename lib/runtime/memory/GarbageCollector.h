//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_GARBAGECOLLECTOR_H
#define SHARP_GARBAGECOLLECTOR_H

#include "../Mutex.h"
#include "../List.h"

enum CollectionPolicy
{
    GC_LOW          =0x001,
    GC_CONCURRENT   =0x002,
    GC_EXPLICIT     =0x003
};

enum CollectionGeneration
{
    gc_young = 0,
    gc_adult = 1,
    gc_old   = 2,

    /**
     * This is a permanent generation. objects promoted to this generation will live
     * for the entirety of the program and will not be garbage collected until the end of
     * the program. Objects in this state must be set manually
     */
    gc_perm  = 3
};

struct Object;
struct SharpObject;
class ClassObject;

class GarbageCollector {
public:
    static GarbageCollector *self;
    Mutex mutex;
    List<CollectionPolicy> messageQueue;

    void initilize();
    void shutdown();
    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    threadStart(void *);
    void run();

    /**
     * Messages are sendable by other threads to command the garbage collector
     * to preform a certain type of garbage collection. Most of the time the user
     * will not have to do this
     * @param message
     */
    void sendMessage(CollectionPolicy message);

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
    void collect(CollectionPolicy policy);

    SharpObject* newObject(unsigned long size); /* Array allocation */
    SharpObject* newObject(unsigned long size, ClassObject* k); /* Class allocation */

    SharpObject* newObjectArray(unsigned long size); /* Array Object allocation */
    SharpObject* newObjectArray(unsigned long size, ClassObject* k); /* Class Array allocation */

    /**
     * Function call by virtual machine
     * @param object
     */
    void freeObject(Object* object);
    CXX11_INLINE void attachObject(Object* object, SharpObject *sharpObject);

    bool spaceAvailable(size_t i);

private:
    unsigned long managedBytes;
    unsigned long memoryLimit;

    /**
     * This will keep track of our different generations and the
     * objects that are living in them.
     *
     * @youngObjects: This will hold the total running count of objects
     * living in that generation
     *
     * @yObjs: This is the amount of dropped objects in each generation since collection.
     * This does not mean that if there are 100 objects dropped that every one will be freed
     * its just an estimate
     */
    unsigned long youngObjects,  yObjs;     /* collect when 10% has been dropped */
    unsigned long adultObjects,  aObjs;     /* collect when 40% has been dropped */
    unsigned long oldObjects,    oObjs;     /* collect when 20% has been dropped */
    List<SharpObject*> heap;

    void collectYoungObjects();
    void collectAdultObjects();
    void collectOldObjects();
    /**
     * This function returns the total ammount of bytes collected
     * @param object
     * @return
     */
    unsigned long collect(SharpObject *object);

    /**
     * Collect a mapped class object or data structure
     * @param pObject
     * @return
     */
    unsigned long collectMappedClass(SharpObject *object, ClassObject *klass);
};

#define GC_SLEEP_INTERVAL 10

/**
 * This number must be low considering that the Garbage collector will
 * not be collecting data every second. We want the garbage collector
 * to be asleep as much as possible.
 */
#define GC_SPIN_MULTIPLIER 512

#define GC_COLLECT_YOUNG() ( (unsigned int)(((double)yObjs/(double)youngObjects)*100) >= 10 )
#define GC_COLLECT_ADULT() ( (unsigned int)(((double)aObjs/(double)adultObjects)*100) >= 40 )
#define GC_COLLECT_OLD() ( (unsigned int)(((double)oObjs/(double)oldObjects)*100) >= 20 )
#define GC_HEAP_LIMIT (MB_TO_BYTES(1))

/**
 * Bytes are used via the JEDEC Standard 100B.01
 */
#define KB_TO_BYTES(bytes) (((unsigned long)bytes)*1024)
#define MB_TO_BYTES(bytes) (((unsigned long)bytes)*1048576)
#define GB_TO_BYTES(bytes) (((unsigned long)bytes)*1073741824)


#endif //SHARP_GARBAGECOLLECTOR_H
