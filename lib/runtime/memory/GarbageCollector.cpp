//
// Created by BraxtonN on 2/11/2018.
//

#include "GarbageCollector.h"
#include "../oo/Object.h"
#include "../Thread.h"
#include "../oo/Field.h"
#include "../../util/time.h"

GarbageCollector *GarbageCollector::self = nullptr;
uint64_t hBytes = 0;

void* __malloc(size_t bytes)
{
    void* ptr =nullptr;
    bool gc=false;
    alloc_bytes:
    if(!GarbageCollector::self->spaceAvailable(bytes))
        goto lowmem;
    ptr=malloc(bytes);

    if(GarbageCollector::self != nullptr && ptr == nullptr) {
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
    void* ptr =nullptr;
    bool gc=false;
    alloc_bytes:
    if(!GarbageCollector::self->spaceAvailable(bytes))
        goto lowmem;
    ptr=calloc(n, bytes);

    if(ptr == nullptr) {
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
    void* rmap =nullptr;
    bool gc=false;
    alloc_bytes:
    if(!GarbageCollector::self->spaceAvailable(bytes))
        goto lowmem;
    rmap=realloc(ptr, bytes);

    if(rmap == nullptr) {
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
#ifdef WIN32_
    self->mutex.initalize();
#endif
#ifdef POSIX_
    self->mutex
#endif
    self->_Mheap = new std::list<SharpObject*>();
    self->managedBytes=0;
    self->memoryLimit = 0;
    self->liveObjects=0;
    self->objs=0;
    self->x = 0;
    self->isShutdown=false;
    self->messageQueue.init();
}

void GarbageCollector::freeObject(Object *object) {
    if(object != nullptr && object->object != nullptr)
    {
        object->object->refCount--;
        objs++;
        object->object = nullptr;
    }
}

void GarbageCollector::markObject(SharpObject *object) {
    if(object != nullptr) {
        object->mark = 0x1;
        if(object->node != NULL) {
            for(unsigned long i = 0; i < object->size; i++) {
                SharpObject *o = object->node[i].object;
                /**
                 * If the object still has references we just drop it and move on
                 */
                if(o != nullptr && o->refCount <= 1) {
                    markObject(o);
                }
            }
        }
    }
}

extern uint64_t past;
extern uint64_t now;
void GarbageCollector::shutdown() {
    if(self != nullptr) {
        managedBytes=0;
        isShutdown=true;
        now= Clock::realTimeInNSecs();
        cout << endl << "Compiled in " << NANO_TOMICRO(now-past) << "us & "
             << NANO_TOMILL(now-past) << "ms\n";

        /* Clear out all memory */
        cout << " sizeof SharpObject: " << sizeof(SharpObject) << endl;
        cout << "highest memory calculated: " << hBytes << endl;
        cout << "memory managed: " << self->managedBytes << endl;
        cout << "Objects Collected " << self->x << endl;
        cout << "Objects left over live objects: " << liveObjects << endl;
        cout << "heap size: " << heap.size() << endl;
        int i = 0;
        for (auto it = heap.begin(); it != heap.end(); it = heap.begin()) {
            sweep(*it);
        }

        delete _Mheap;
        std::free(self); self = nullptr;
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
        if(managedBytes > hBytes)
            hBytes = managedBytes;

        /**
         * This should only be called by the GC thread itsself
         */
        if(objs>0) collectYoungObjects();
//        if(GC_COLLECT_ADULT()) collectAdultObjects();
//        if(GC_COLLECT_OLD()) collectOldObjects();
    }
}

void GarbageCollector::collectYoungObjects() {
    objs = 0;

    std::lock_guard<recursive_mutex> gd(mutex);
    for (auto it = heap.begin(); it != heap.end();) {
        SharpObject *object = *it;

        if(thread_self->state == THREAD_KILLED) {
            return;
        }

        // mark object
        if(object->refCount == 0) {
           // markObject(object);
            sweep(object);
            it = iterator;
        } else {
            ++it;
        }
    }

}

void GarbageCollector::collectAdultObjects() {
//    aObjs = 0;
//
//    std::lock_guard<recursive_mutex> gd(mutex);
//    for (auto it = heap.begin(); it != heap.end();) {
//        SharpObject *object = *it;
//
//        if(thread_self->state == THREAD_KILLED) {
//            return;
//        }
//
//        if(GENERATION(object->_gcInfo) == gc_adult) {
//
//            // mark object
//            if(object->refCount == 0) {
//                markObject(object);
//                sweep(object);
//                it = iterator;
//            } else if(IS_MARKED(object->_gcInfo)) {
//                sweep(object);
//                it = iterator;
//            } else if(object->refCount > 0){
//                adultObjects--;
//                oldObjects++;
//                SET_GENERATION(object->_gcInfo, gc_old);
//                ++it;
//            } else {
//                cout << "wtf\n";
//                ++it;
//            }
//        } else
//            ++it;
//    }

}

void GarbageCollector::collectOldObjects() {
//    oObjs = 0;
//
//    std::lock_guard<recursive_mutex> gd(mutex);
//    for (auto it = heap.begin(); it != heap.end();) {
//        SharpObject *object = *it;
//
//        if(thread_self->state == THREAD_KILLED) {
//            return;
//        }
//
//        if(GENERATION(object->_gcInfo) == gc_old) {
//
//            // mark object
//            if(object->refCount == 0) {
//                markObject(object);
//                sweep(object);
//                it = iterator;
//            } else if(IS_MARKED(object->_gcInfo)) {
//                sweep(object);
//                it = iterator;
//            } else if(object->refCount > 0){
//                /* We dont care */
//                ++it;
//            } else {
//                cout << "wtf\n";
//                ++it;
//            }
//        } else
//            ++it;
//    }
}

void GarbageCollector::run() {

    const unsigned int sMaxRetries = 128 * 512;
    unsigned int retryCount = 0;

    long maxRetries = 10000000;
    long spincount = 0;

    for(;;) {
        if(thread_self->suspendPending)
            Thread::suspendSelf();
        if(thread_self->state == THREAD_KILLED) {
            return;
        }

        if(!messageQueue.empty()) {
            mutex.lock();
            CollectionPolicy policy = messageQueue.last();
            messageQueue.pop_back();
            mutex.unlock();

            /**
             * We only want to run a concurrent collection
             * in the GC thread itsself
             */
            if(policy != GC_CONCURRENT)
                collect(policy);
        }

//        if(spincount++ >= maxRetries) {
//            // sleep for 4 milliseconds
//            __os_sleep(4);
//        }

        if (retryCount++ == sMaxRetries)
        {
            retryCount = 0;
#ifdef WIN32_
            //Sleep(4);
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
    return nullptr;
#endif
}

void GarbageCollector::sendMessage(CollectionPolicy message) {
    std::lock_guard<recursive_mutex> gd(mutex);
    messageQueue.push_back(message);
}

void GarbageCollector::sweep(SharpObject *object) {
    if(object != nullptr) {

        if (object->data != nullptr) {
            managedBytes -= sizeof(double) * object->size;
            std::free(object->data);
        } else if (object->node != nullptr) {
            for (unsigned long i = 0; i < object->size; i++) {
                SharpObject *o = object->node[i].object;
                /**
                 * If the object still has references we just drop it and move on
                 */
                if (o != nullptr && o->refCount <= 1) {
                    sweep(o);
                } else
                    freeObject(&object->node[i]);
            }

            managedBytes -= sizeof(Object) * object->size;
            std::free(object->node);
        }

        UPDATE_GC(object)
        x++;

        managedBytes -= sizeof(SharpObject) * 1;
        std::free(object);
        invalidate(object);
    }
}

SharpObject *GarbageCollector::newObject(unsigned long size) {
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

    object->init();
    object->size = size;
    object->refCount=1;
    if(size > 0) {
        object->data = (double*)__calloc(size, sizeof(double));
    }

    /* track the allocation amount */
    std::lock_guard<recursive_mutex> gd(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(double)*size);
    heap.push_back(object);
    liveObjects++;

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

        object->init();
        object->size = k->fieldCount;
        object->refCount=1;
        object->k = k;

        if(k->fieldCount > 0) {
            object->node = (Object*)__calloc(k->fieldCount, sizeof(Object));
            for(unsigned int i = 0; i < object->size; i++) {
                /**
                 * We want to set the class variables and arrays
                 * to null and initialize the var variables
                 */
                if(k->fields[i].type == VAR && !k->fields[i].isArray) {
                    object->node[i].object = newObject(1);
                } else {
                    object->node[i].object = nullptr;
                }
            }

        }

        std::lock_guard<recursive_mutex> gd(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*k->fieldCount);
        heap.push_back(object);
        liveObjects++;
        return object;
    }

    return nullptr;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size) {
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

    object->init();
    object->size = size;
    object->refCount=1;
    if(size > 0) {
        object->node = (Object*)__malloc(sizeof(Object)*size);
        for(unsigned int i = 0; i < object->size; i++)
            object->node[i].object = nullptr;
    }

    /* track the allocation amount */
    std::lock_guard<recursive_mutex> gd(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
    heap.push_back(object);
    liveObjects++;

    return object;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size, ClassObject *k) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);

        object->init();
        object->size = size;
        object->refCount=1;
        object->k = k;
        if(size > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*size);
            for(unsigned int i = 0; i < object->size; i++)
                object->node[i].object = nullptr;
        }

        /* track the allocation amount */
        std::lock_guard<recursive_mutex> gd(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
        heap.push_back(object);
        liveObjects++;

        return object;
    }

    return nullptr;
}

void GarbageCollector::createStringArray(Object *object, native_string& s) {
    if(object != nullptr) {
        freeObject(object);
        object->object = newObject(s.len);

        for(unsigned long i = 0; i < s.len; i++) {
            object->object->data[i] = s.chars[i];
        }
    }
}

void GarbageCollector::createStringArray(native_string& s) {
    SharpObject *object = newObject(s.len);
    thread_self->dataStack->push(object);

    for(unsigned long i = 0; i < s.len; i++) {
        object->data[i] = s.chars[i];
    }

}
