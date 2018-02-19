//
// Created by BraxtonN on 2/11/2018.
//

#include "GarbageCollector.h"
#include "../oo/Object.h"
#include "../Thread.h"
#include "../oo/Field.h"

static GarbageCollector *self = NULL;

void* __malloc(size_t bytes)
{
    void* ptr =NULL;
    bool gc=false;
    alloc_bytes:
    if(!self->spaceAvailable(bytes))
        goto lowmem;
    ptr=malloc(bytes);

    if(GarbageCollector::self != NULL && ptr == NULL) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc = true;
            self->collect(GC_LOW);
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
    if(!self->spaceAvailable(bytes))
        goto lowmem;
    ptr=calloc(n, bytes);

    if(ptr == NULL) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc=true;
            self->collect(GC_LOW);
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
    if(!self->spaceAvailable(bytes))
        goto lowmem;
    rmap=realloc(ptr, bytes);

    if(rmap == NULL) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc=true;
            self->collect(GC_LOW);
            goto alloc_bytes;
        }
    } else {
        return rmap;
    }
}

void GarbageCollector::initilize() {
    self=(GarbageCollector*)malloc(sizeof(GarbageCollector)*1);
    self->mutex = Mutex();
    self->heap.init();
    self->managedBytes=0;
    self->adultObjects=0;
    self->youngObjects=0;
    self->oldObjects=0;
    self->yObjs=0;
    self->aObjs=0;
    self->oObjs=0;
    self->isShutdown=false;
}

CXX11_INLINE
void GarbageCollector::freeObject(Object *object) {
    if(object != NULL && object->object != NULL)
    {
        object->object->mutex.acquire(INDEFINITE);
        object->object->refCount--;

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

        object->object->mutex.release();
        object->object = NULL;
    }
}

CXX11_INLINE
void GarbageCollector::attachObject(Object* object, SharpObject *sharpObject) {
    if(object != NULL && sharpObject != NULL) {
        sharpObject->mutex.acquire(INDEFINITE);
        sharpObject->refCount++;
        object->object = sharpObject;
        sharpObject->mutex.release();
    }
}

void GarbageCollector::shutdown() {
    if(self != NULL) {
        managedBytes=0;
        /* Clear out all memory */
        for(unsigned int i = 0; i < heap.size(); i++) {
            collect(heap.get(i));
            std::free(heap.get(i));
        }
        heap.free();
        std::free(self); self = NULL;
        isShutdown=true;
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

    mutex.acquire(INDEFINITE);

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

    mutex.release();
}

void GarbageCollector::collectYoungObjects() {
    SharpObject *object;
    yObjs = 0;

    reset:
    for(unsigned int i = 0; i < heap.size(); i++) {
        object = heap.get(i);

        if(object->generation == gc_young) {
            /* If all threads are suspended we don't have to worry about any interference */
            if(!Thread::isAllThreadsSuspended)
                object->mutex.acquire(INDEFINITE);

            // free object
            if(object->refCount == 0) {
                collect(object);
                youngObjects--;
                std::free(object);
                heap.remove(i); // drop pointer and reset list
                goto reset;
            } else {
                adultObjects++;

                object->generation = gc_adult;
                if(!Thread::isAllThreadsSuspended)
                    object->mutex.release();
            }

        }
    }
}

void GarbageCollector::collectAdultObjects() {
    SharpObject *object;
    aObjs = 0;

    reset:
    for(unsigned int i = 0; i < heap.size(); i++) {
        object = heap.get(i);

        if(object->generation == gc_adult) {
            /* If all threads are suspended we don't have to worry about any interference */
            if(!Thread::isAllThreadsSuspended)
                object->mutex.acquire(INDEFINITE);

            // free object
            if(object->refCount == 0) {
                collect(object);
                adultObjects--;
                std::free(object);
                heap.remove(i); // drop pointer and reset list
                goto reset;
            } else {
                oldObjects++;

                object->generation = gc_old;
                if(!Thread::isAllThreadsSuspended)
                    object->mutex.release();
            }

        }
    }
}

void GarbageCollector::collectOldObjects() {
    SharpObject *object;
    oObjs = 0;

    reset:
    for(unsigned int i = 0; i < heap.size(); i++) {
        object = heap.get(i);

        if(object->generation == gc_old) {
            /* If all threads are suspended we don't have to worry about any interference */
            if(!Thread::isAllThreadsSuspended)
                object->mutex.acquire(INDEFINITE);

            // free object
            if(object->refCount == 0) {
                collect(object);
                oldObjects--;
                std::free(object);
                heap.remove(i); // drop pointer and reset list
                goto reset;
            } else {
                /**
                 * We are already at the highest generation so we just skip this
                 * object
                 */
                if(!Thread::isAllThreadsSuspended)
                    object->mutex.release();
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
            mutex.acquire(INDEFINITE);
            CollectionPolicy policy = messageQueue.last();
            messageQueue.pop_back();

            /**
             * We only want to run a concurrent collection
             * in the GC thread itsself
             */
            if(policy != GC_CONCURRENT)
                collect(policy);
            else
                mutex.release();
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
    cout << "gc exited" << endl;
#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return NULL;
#endif
}

void GarbageCollector::sendMessage(CollectionPolicy message) {
    mutex.acquire(INDEFINITE);
    messageQueue.push_back(message);
    mutex.release();
}

void GarbageCollector::collect(SharpObject *object) {
    if(object != NULL) {

        if(object->size > 0) {
            if(object->HEAD != NULL) {
                managedBytes -= sizeof(double)*object->size;
                std::free(object->HEAD); object->HEAD = NULL;
            } else if(object->node != NULL) {
                for(unsigned long i = 0; i < object->size; i++) {
                    /**
                     * If the object still has references we just drop it and move on
                     */
                    if(object->node[i].object->refCount > 1)
                        freeObject(&object->node[i]);
                    else
                        collect(object->node[i].object);
                }

                managedBytes -= sizeof(Object)*object->size;
                std::free(object->node);
            }
        }
    }
}

bool GarbageCollector::spaceAvailable(size_t bytes) {
    return (bytes+managedBytes) < memoryLimit;
}

SharpObject *GarbageCollector::newObject(unsigned long size) {
    auto *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

    object->init();
    object->size = size;
    object->refCount=1;
    if(size > 0) {
        object->HEAD = (double*)__malloc(sizeof(double)*size);
        for(unsigned int i = 0; i < object->size; i++)
            object->HEAD[i]=0;

        managedBytes += (sizeof(double)*size);
    }

    /* track the allocation amount */
    managedBytes += (sizeof(SharpObject)*1);
    heap.add(object);

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k) {
    if(k != NULL) {
        auto *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

        object->init();
        object->size = k->fieldCount;
        object->refCount=1;

        if(k->fieldCount > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*k->fieldCount);
            for(unsigned int i = 0; i < object->size; i++) {
                /**
                 * We want to set the class variables and arrays
                 * to null and initialize the var variables
                 */
                if(k->fields[i].type == VAR && !k->fields[i].isArray) {
                    object->node[i].object = newObject(1);
                } else
                    object->node[i].object=NULL;
            }

            managedBytes += (sizeof(Object)*k->fieldCount);
        }

        managedBytes += (sizeof(SharpObject)*1);
        return object;
    }

    return NULL;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size) {
    auto *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

    object->init();
    object->size = size;
    object->refCount=1;
    if(size > 0) {
        object->node = (Object*)__malloc(sizeof(Object)*size);
        for(unsigned int i = 0; i < object->size; i++)
            object->node[i].object = NULL;

        managedBytes += (sizeof(Object)*size);
    }

    /* track the allocation amount */
    managedBytes += (sizeof(SharpObject)*1);
    heap.add(object);

    return object;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size, ClassObject *k) {
    if(k != NULL) {
        auto *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

        object->init();
        object->size = size;
        object->refCount=1;
        object->k = k;
        if(size > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*size);
            for(unsigned int i = 0; i < object->size; i++)
                object->node[i].object = NULL;

            managedBytes += (sizeof(Object)*size);
        }

        /* track the allocation amount */
        managedBytes += (sizeof(SharpObject)*1);
        heap.add(object);

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
