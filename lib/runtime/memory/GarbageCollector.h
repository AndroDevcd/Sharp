//
// Created by BraxtonN on 2/11/2018.
//

#ifndef SHARP_GARBAGECOLLECTOR_H
#define SHARP_GARBAGECOLLECTOR_H

#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include "../../../stdimports.h"
#include "../List.h"
#include "../../util/KeyPair.h"
#include "../symbols/string.h"

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

enum GCState 
{
    CREATED,
    RUNNING,
    SLEEPING,
    KILLED,
    SHUTDOWN
};

struct Object;
struct SharpObject;
class ClassObject;
class Thread;

struct mutex_t
{
    SharpObject* object;
    uInt lockedCount;
    long threadid;
    
    mutex_t(SharpObject *o, recursive_mutex *mut, long threadid) 
    : 
        object(o),
        threadid(threadid),
        lockedCount(0)
    {
    }
};

#define heap (_Mheap)
class GarbageCollector;
extern GarbageCollector gc;

class GarbageCollector {
public:
    GarbageCollector() 
    :
        state(CREATED),
        mutex(),
        tself(NULL),
        messageQueue(),
        managedBytes(0),
        memoryLimit(0),
        memoryThreshold(0),
        youngObjects(0),
        adultObjects(0),
        oldObjects(0),
#ifdef SHARP_PROF_
        x(),
        largestCollectionTime(),
        collections(),
        timeSpentCollecting(),
        timeSlept(),
#endif
        locks(),
        _Mheap(NULL),
        tail(NULL),
        heapSize(0)
    {
    }

    GCState state;
    recursive_mutex mutex;
    Thread *tself;
    _List<CollectionPolicy> messageQueue;

    static void initilize();
    static void startup();
    static void shutdown();
    static
#ifdef WIN32_
    DWORD WINAPI
#endif
#ifdef POSIX_
    void*
#endif
    threadStart(void *);
    void run();

    static void setMemoryLimit(uInt limit) {
        gc.memoryLimit = limit;
    }

    static void setMemoryThreshold(uInt limit) {
        gc.memoryThreshold = limit;
    }

    bool isShutdown() {
        return state == SHUTDOWN;
    }

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

    SharpObject* newObject(int64_t size, unsigned short ntype); /* Array allocation */
    SharpObject* newObjectUnsafe(int64_t size, unsigned short ntype); /* Unconventional Array allocation */
    SharpObject* newObject(ClassObject* k, bool staticInit = false); /* Class allocation */
    SharpObject* newObjectUnsafe(ClassObject* k, bool staticInit = false); /* Class allocation */

    SharpObject* newObjectArray(int64_t size); /* Array Object allocation */
    SharpObject* newObjectArray(int64_t size, ClassObject* k); /* Class Array allocation */

    void createStringArray(Object* object, runtime::String& str); /* Native string allocation */

    /**
     * Utility system level functions to garbage collect at a high level
     *
     * The functions below give the user the power to perform self collections
     * starting and stoping the GC, etc.
     */
    int selfCollect();
    void sedate();
    void wake();
    void kill();
    bool isAwake();

    /**
     * Reallocation methods for faster code
     * @param o
     * @param sz
     */
    void realloc(SharpObject *o, size_t sz);
    void reallocObject(SharpObject *o, size_t sz);

    /**
     * Function call by virtual machine
     * @param object
     */
     void releaseObject(Object *object);

     /**
      * Add untracked memory to managed memory
      * @param bytes
      */
     CXX11_INLINE void addMemory(size_t bytes) {
         managedBytes += bytes;
     }


    /**
     * Removed tracked memory from managed memory
     * @param bytes
     */
    CXX11_INLINE void freeMemory(size_t bytes) {
        mutex.lock();
        managedBytes -= bytes;
        mutex.unlock();
    }

    CXX11_INLINE bool spaceAvailable(uInt bytes) {
        return (bytes+managedBytes) < memoryLimit;
    }

    uInt getMemoryLimit();
    uInt getManagedMemory();

    /**
     * Retrieve the os level size of an object refrence
     * @param object
     * @return
     */
    static double _sizeof(SharpObject *object);

    /**
     * Lock an object to be thread safe
     *
     */
    bool lock(SharpObject *, Thread*);
    void unlock(SharpObject *);
    
    /**
     * Thread sycronization protection for destroyed threads with pending locks
     */
    void reconcileLocks(Thread*);

    void printClassRefStatus();
    SharpObject *getObjectAt(uInt index);
private:
    std::atomic<uInt> managedBytes;
    std::atomic<uInt> memoryLimit;
    std::atomic<uInt> memoryThreshold;

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
    std::atomic<uInt> youngObjects;
    std::atomic<uInt> adultObjects;
    std::atomic<uInt> oldObjects;
#ifdef SHARP_PROF_
    unsigned long x;
    unsigned long largestCollectionTime;
    unsigned long collections;
    unsigned long timeSpentCollecting;
    unsigned long timeSlept;
#endif
    _List<mutex_t*> locks;
    SharpObject* _Mheap, *tail;
    std::atomic<uInt> heapSize;

    void collectGarbage(bool markOnly = false);

    /**
     * This function performs the actual collection of
     * Sharp objects in the heap
     * @param object
     * @return
     */
    SharpObject* sweep(SharpObject *object, SharpObject *prevObj);

    void sedateSelf();

    void dropLock(SharpObject *);

    void updateMemoryThreshold();
};

#define GC_COLLECT_MEM() ( managedBytes >= memoryThreshold )
#define GC_HEAP_LIMIT (MB_TO_BYTES(250))

// object info macros
#define GENERATION_MASK 0x3
#define CLASS_MASK 0xffffff
#define TYPE_MASK 0x7
#define GENERATION(inf) ((inf >> 28) & GENERATION_MASK)
#define CLASS(inf) (inf & CLASS_MASK)
#define TYPE(inf) ((uint32_t)(inf >> 24) & TYPE_MASK)
#define MARKED(inf) ((inf >> 30) & 1UL)
#define HAS_LOCK(inf) ((inf >> 31))
#define IS_CLASS(inf) ((inf >> 27) & 1UL)
#define MARK(inf, enable) (inf ^= (-(unsigned long)enable ^ inf) & (1UL << 30))
#define SET_LOCK(inf, enable) (inf ^= (-(unsigned long)enable ^ inf) & (1UL << 31))
#define SET_CLASS_BIT(inf, enable) (inf ^= (-(unsigned long)enable ^ inf) & (1UL << 27))
#define SET_GENERATION(inf, gen) (inf = (uint32_t)(CLASS(inf) | (TYPE(inf) << 24) | (IS_CLASS(inf) << 27) | (gen << 28) | (MARKED(inf) << 30) | (HAS_LOCK(inf) << 31)))
#define SET_TYPE(inf, type) (inf = (uint32_t)(CLASS(inf) | (type << 24) | ((inf >> 27) << 27)))
#define SET_INFO(inf, k, type, gen) (inf = (uint32_t)(k | (type << 24) |  (gen << 27)))

#define UPDATE_GC(object) \
    switch(GENERATION(object->info)) { \
        case gc_young: \
            youngObjects--; \
            break; \
        case gc_adult: \
            adultObjects--; \
            break; \
        case gc_old: \
            oldObjects--; \
            break; \
    }

#define PUSH(object) { \
    heapSize++; \
    tail->next = object; \
    tail = object; \
}

/**
 * Bytes are used via the JEDEC Standard 100B.01
 */
#define KB_TO_BYTES(bytes) (((uInt)bytes)*1024)
#define MB_TO_BYTES(bytes) (((uInt)bytes)*1048576)
#define GB_TO_BYTES(bytes) (((uInt)(bytes))*1073741824)

#define GUARD(mut) \
    std::lock_guard<recursive_mutex> guard(mut);


#endif //SHARP_GARBAGECOLLECTOR_H
