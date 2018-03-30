//
// Created by BraxtonN on 2/11/2018.
//

#include "GarbageCollector.h"
#include "../oo/Object.h"
#include "../Thread.h"
#include "../oo/Field.h"

GarbageCollector *GarbageCollector::self = NULL;

void* __malloc(size_t bytes)
{
    void* ptr =NULL;
    bool gc=false;
    alloc_bytes:
    if(!GarbageCollector::self->spaceAvailable(bytes))
        goto lowmem;
    ptr=malloc(bytes);

    if(GarbageCollector::self != NULL && ptr == NULL) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc = true;
            GarbageCollector::self->collect(GC_LOW);
            goto alloc_bytes;
        }
    } else {
        return ptr;
    }
}
void* __calloc(size_t n, size_t bytes)
{
    void* ptr =NULL;
    bool gc=false;
    alloc_bytes:
    if(!GarbageCollector::self->spaceAvailable(bytes))
        goto lowmem;
    ptr=calloc(n, bytes);

    if(ptr == NULL) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc=true;
            GarbageCollector::self->collect(GC_LOW);
            goto alloc_bytes;
        }
    } else {
        return ptr;
    }
}
void* __realloc(void *ptr, size_t bytes)
{
    void* rmap =NULL;
    bool gc=false;
    alloc_bytes:
    if(!GarbageCollector::self->spaceAvailable(bytes))
        goto lowmem;
    rmap=realloc(ptr, bytes);

    if(rmap == NULL) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc=true;
            GarbageCollector::self->collect(GC_LOW);
            goto alloc_bytes;
        }
    } else {
        return rmap;
    }
}

void GarbageCollector::initilize() {
    self=(GarbageCollector*)malloc(sizeof(GarbageCollector)*1);
    self->mutex = NULL;
    MUTEX_INIT(&self->mutex);
    self->heap.init();
    self->managedBytes=0;
    self->memoryLimit = 0;
    self->adultObjects=0;
    self->youngObjects=0;
    self->oldObjects=0;
    self->yObjs=0;
    self->aObjs=0;
    self->oObjs=0;
    self->isShutdown=false;
    self->messageQueue.init();
}

void GarbageCollector::freeObject(Object *object) {
    if(object != NULL && object->object != NULL)
    {
        MUTEX_LOCK(object->object->mutex)
        object->object->refCount--;

        MUTEX_LOCK(mutex)
        switch(object->object->generation) {
            case gc_young:
                yObjs++;
                break;
            case gc_adult:
                aObjs++;
                break;
            case gc_old:
                oObjs++;
                break;
        }
        MUTEX_UNLOCK(mutex)

        MUTEX_UNLOCK(object->object->mutex)
        object->object = NULL;
    }
}

void GarbageCollector::attachObject(Object* object, SharpObject *sharpObject) {
    if(object != NULL && sharpObject != NULL) {
        MUTEX_LOCK(sharpObject->mutex)
        sharpObject->refCount++;
        object->object = sharpObject;
        MUTEX_UNLOCK(sharpObject->mutex)
    }
}

void GarbageCollector::shutdown() {
    if(self != NULL) {
        managedBytes=0;
        isShutdown=true;
        /* Clear out all memory */
        for(unsigned int i = 0; i < heap.size(); i++) {
            SharpObject *o=heap.get(i);
            collect(o);
        }
        heap.free();
        std::free(self); self = NULL;
    }
}

void GarbageCollector::startup() {
    auto* gcThread=(Thread*)malloc(
            sizeof(Thread)*1);
    gcThread->CreateDaemon("gc");
    Thread::startDaemon(
            GarbageCollector::threadStart, gcThread);
}

void GarbageCollector::collect(CollectionPolicy policy) {
    if(isShutdown)
        return;

    if(policy == GC_LOW) {
        Thread::suspendAllThreads();

        /**
         * To attempt to approve a large memory request we want to take the
         * worst case scenareo route to try to fuffil the memory request. To
         * avoid lags in the application big memory requests should not be
         * performed often
         */
        collectYoungObjects();
        collectAdultObjects();
        collectOldObjects();

        Thread::resumeAllThreads();
    } else if(policy == GC_EXPLICIT) {
        /**
         * Force collection of both generations
         * We only do the first 2 generations because we want
         * to prevent lag when freeing up memory. the Old generation will
         * always be the largest generation
         */
            collectYoungObjects();
            collectAdultObjects();
    } else if(policy == GC_CONCURRENT) {
        /**
         * This should only be called by the GC thread itsself
         */
        if(GC_COLLECT_YOUNG()) {
            collectYoungObjects();
        }
        if(GC_COLLECT_ADULT()) {
            collectAdultObjects();
        }
        if(GC_COLLECT_OLD()) {
            collectOldObjects();
        }
    }

    MUTEX_UNLOCK(mutex)
}

void GarbageCollector::collectYoungObjects() {
    yObjs = 0;

    reset:
    for(unsigned int i = 0; i < heap.size(); i++) {

        MUTEX_LOCK(mutex)
        SharpObject *object = heap._Data[i];

        if(object != NULL && object->generation == gc_young) {
            /* If all threads are suspended we don't have to worry about any interference */
            if(!Thread::isAllThreadsSuspended)
                MUTEX_LOCK(object->mutex)

            // free object
            if(object->refCount == 0) {
                collect(object);
                youngObjects--;
                heap._Data[i]=NULL; // drop pointer and reset list
                MUTEX_UNLOCK(mutex)
                goto reset;
            } else {
                adultObjects++;

                object->generation = gc_adult;
                if(!Thread::isAllThreadsSuspended)
                    MUTEX_UNLOCK(object->mutex)
                MUTEX_UNLOCK(mutex)
            }

        }
    }
}

void GarbageCollector::collectAdultObjects() {
    SharpObject *object;
    aObjs = 0;

    reset:
    for(unsigned int i = 0; i < heap.size(); i++) {

        MUTEX_LOCK(mutex)
        object = heap._Data[i];

        if(object != NULL && object->generation == gc_adult) {
            /* If all threads are suspended we don't have to worry about any interference */
            if(!Thread::isAllThreadsSuspended)
                MUTEX_LOCK(object->mutex)

            // free object
            if(object->refCount == 0) {
                collect(object);
                adultObjects--;
                heap._Data[i]=NULL; // drop pointer and reset list
                MUTEX_UNLOCK(mutex)
                goto reset;
            } else {
                oldObjects++;

                object->generation = gc_old;
                if(!Thread::isAllThreadsSuspended)
                    MUTEX_UNLOCK(object->mutex)
                MUTEX_UNLOCK(mutex)
            }

        }
    }
}

void GarbageCollector::collectOldObjects() {
    SharpObject *object;
    oObjs = 0;

    reset:
    for(unsigned int i = 0; i < heap.size(); i++) {
        MUTEX_LOCK(mutex)
        object = heap._Data[i];

        if(object != NULL && object->generation == gc_old) {
            /* If all threads are suspended we don't have to worry about any interference */
            if(!Thread::isAllThreadsSuspended)
                MUTEX_LOCK(object->mutex)

            // free object
            if(object->refCount == 0) {
                collect(object);
                oldObjects--;
                heap._Data[i]=NULL; // drop pointer and reset list
                MUTEX_UNLOCK(mutex)
                goto reset;
            } else {
                /**
                 * We are already at the highest generation so we just skip this
                 * object
                 */
                if(!Thread::isAllThreadsSuspended)
                    MUTEX_UNLOCK(object->mutex)
                MUTEX_UNLOCK(mutex)
            }

        }
    }
}

void GarbageCollector::run() {
    const unsigned int sMaxRetries = 128 * GC_SPIN_MULTIPLIER;
    unsigned int retryCount = 0;

    for(;;) {
        if(thread_self->suspendPending)
            Thread::suspendSelf();
        if(thread_self->state == THREAD_KILLED)
            return;

        if(!messageQueue.empty()) {
            MUTEX_LOCK(mutex)
            CollectionPolicy policy = messageQueue.last();
            messageQueue.pop_back();
            MUTEX_UNLOCK(mutex);

            /**
             * We only want to run a concurrent collection
             * in the GC thread itsself
             */
            if(policy != GC_CONCURRENT)
                collect(policy);
        }

        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
#ifdef WIN32_
            Sleep(GC_SLEEP_INTERVAL);
#endif
#ifdef POSIX_
            usleep(GC_SLEEP_INTERVAL*POSIX_USEC_INTERVAL);
#endif
        } else {
            /**
             * Attempt to collect objects based on the appropriate
             * conditions. This call does not garuntee that any collections
             * will happen
             */
            collect(GC_CONCURRENT);
        }

    }
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
GarbageCollector::threadStart(void *pVoid) {
    thread_self =(Thread*)pVoid;
    thread_self->state = THREAD_RUNNING;

    try {
        self->run();
    } catch(Exception &e){
        /* Should never happen */
        thread_self->exceptionThrown =true;
        thread_self->throwable=e.getThrowable();
    }

        /*
         * Check for uncaught exception in thread before exit
         */
    thread_self->exit();
#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return NULL;
#endif
}

void GarbageCollector::sendMessage(CollectionPolicy message) {
    MUTEX_LOCK(mutex)
    messageQueue.push_back(message);
    MUTEX_UNLOCK(mutex)
}

void GarbageCollector::collect(SharpObject *object) {
    if(object != NULL) {

        if(object->size > 0) {
            if(object->HEAD != NULL) {
                managedBytes -= sizeof(double)*object->size;
                std::free(object->HEAD); object->HEAD = NULL;
            } else if(object->node != NULL) {
                if(!isShutdown) {
                    for(unsigned long i = 0; i < object->size; i++) {
                        /**
                         * If the object still has references we just drop it and move on
                         */
                        freeObject(&object->node[i]);
                    }
                }


                managedBytes -= sizeof(Object)*object->size;
                std::free(object->node);
            }
        }

        std::free(object);
        managedBytes -= sizeof(SharpObject)*1;
    }
}

bool GarbageCollector::spaceAvailable(size_t bytes) {
    return (bytes+managedBytes) < memoryLimit;
}

SharpObject *GarbageCollector::newObject(unsigned long size) {
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

    object->init();
    object->size = size;
    object->refCount=1;
    if(size > 0) {
        object->HEAD = (double*)__calloc(size, sizeof(double));

        MUTEX_LOCK(mutex)
        managedBytes += (sizeof(double)*size);
    }

    /* track the allocation amount */
    MUTEX_LOCK(mutex)
    managedBytes += (sizeof(SharpObject)*1);
    heap.addReplace(NULL, object);
    youngObjects++;
    MUTEX_UNLOCK(mutex);

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k) {
    if(k != NULL) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

        object->init();
        object->size = k->fieldCount;
        object->refCount=1;
        object->k = k;

        MUTEX_LOCK(mutex);
        if(k->fieldCount > 0) {
            object->node = (Object*)__calloc(k->fieldCount, sizeof(Object));
            for(unsigned int i = 0; i < object->size; i++) {
                /**
                 * We want to set the class variables and arrays
                 * to null and initialize the var variables
                 */
                if(k->fields[i].type == VAR && !k->fields[i].isArray) {
                    object->node[i].object = newObject(1);
                }
            }

            managedBytes += (sizeof(Object)*k->fieldCount);
        }

        managedBytes += (sizeof(SharpObject)*1);
        heap.addReplace(NULL, object);
        youngObjects++;
        MUTEX_UNLOCK(mutex);
        return object;
    }

    return NULL;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size) {
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

    object->init();
    object->size = size;
    object->refCount=1;
    if(size > 0) {
        object->node = (Object*)__malloc(sizeof(Object)*size);
        for(unsigned int i = 0; i < object->size; i++)
            object->node[i].object = NULL;

        MUTEX_LOCK(mutex)
        managedBytes += (sizeof(Object)*size);
    }

    /* track the allocation amount */
    MUTEX_LOCK(mutex)
    managedBytes += (sizeof(SharpObject)*1);
    heap.addReplace(NULL, object);
    youngObjects++;
    MUTEX_UNLOCK(mutex)

    return object;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size, ClassObject *k) {
    if(k != NULL) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

        object->init();
        object->size = size;
        object->refCount=1;
        object->k = k;
        if(size > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*size);
            for(unsigned int i = 0; i < object->size; i++)
                object->node[i].object = NULL;

            MUTEX_LOCK(mutex)
            managedBytes += (sizeof(Object)*size);
        }

        /* track the allocation amount */
        MUTEX_LOCK(mutex)
        managedBytes += (sizeof(SharpObject)*1);
        heap.addReplace(NULL, object);
        youngObjects++;
        MUTEX_UNLOCK(mutex)

        return object;
    }

    return NULL;
}

void GarbageCollector::createStringArray(Object *object, native_string& s) {
    if(object != NULL) {
        freeObject(object);
        object->object = newObject(s.len);

        for(unsigned long i = 0; i < s.len; i++) {
            object->object->HEAD[i] = s.chars[i];
        }
    }
}
