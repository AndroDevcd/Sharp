//
// Created by BraxtonN on 2/11/2018.
//

#include <random>
#include "GarbageCollector.h"
#include "../oo/Object.h"
#include "../Thread.h"
#include "../oo/Field.h"
#include "../../util/time.h"
#include "../../util/KeyPair.h"
#include <thread>
#include <algorithm>

long long hbytes;
GarbageCollector *GarbageCollector::self = nullptr;

void* __malloc(size_t bytes)
{
    void* ptr =nullptr;
    bool gc=false;
    alloc_bytes:
    if(GarbageCollector::self != nullptr && !GarbageCollector::self->spaceAvailable(bytes))
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
    if(GarbageCollector::self != nullptr && !GarbageCollector::self->spaceAvailable(n*bytes))
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
    if(GarbageCollector::self != nullptr && !GarbageCollector::self->spaceAvailable(bytes))
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
    self->dirtyMutex.initalize();
#endif
#ifdef POSIX_
    new (&self->mutex) std::mutex();
    new (&self->dirtyMutex) std::mutex();
#endif
    new (&self->_Mheap) std::vector<SharpObject*>();
    new (&self->dirtyList) std::vector<SharpObject*>();
    self->managedBytes=0;
    self->memoryLimit = 0;
    self->adultObjects=0;
    self->youngObjects=0;
    self->oldObjects=0;
    self->yObjs=0;
    self->x = 0;
    self->aObjs=0;
    self->oObjs=0;
    self->isShutdown=false;
    self->messageQueue.init();
}

void GarbageCollector::freeObject(Object *object) {
    if(object != nullptr && object->object != nullptr)
    {
        object->object->refCount--;

        switch(GENERATION(object->object->generation)) {
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
        object->object = nullptr;
    }
}

void GarbageCollector::shutdown() {
    if(self != nullptr) {
        isShutdown=true;
        /* Clear out all memory */
        cout << "size of object: " << sizeof(SharpObject) << endl;
        cout << "highest memory calculated: " << hbytes << endl;
        cout << "Objects Collected " << self->x << endl;
        cout << "Total managed bytes left " << self->managedBytes << endl;
        cout << "Objects left over young: " << youngObjects << " adult: " << adultObjects
                                          << " old: " << oldObjects << endl;
        cout << "heap size: " << heap.size() << endl;
        cout << std::flush << endl;

        SharpObject *p;
        for (auto it = heap.begin(); it != heap.end(); ++it) {
            p = *it;


            if(p->refCount < 1)
                it = sweep(p);
        }

        _Mheap.clear();
        std::free(self); self = nullptr;
        managedBytes=0;
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
        if(GC_COLLECT_YOUNG()) {        /* 10% */
            collectYoungObjects();
        }
        if(GC_COLLECT_ADULT()) {        /* 40% */
            collectAdultObjects();
        }
        if(GC_COLLECT_OLD()) {        /* 20% */
            collectOldObjects();
        }
    }
}

void GarbageCollector::collectYoungObjects() {

    mutex.lock();
    yObjs = 0;

    SharpObject *object;
    for (auto it = heap.begin(); it != heap.end(); ++it) {
        object = *it;

        if(tself->state == THREAD_KILLED) {
            break;
        }

        if(GENERATION(object->generation) == gc_young) {
            // free object
            if(MARKED(object->generation) && object->refCount == 0) {
                it = sweep(object);
            } else if(MARKED(object->generation) && object->refCount > 0){
                youngObjects--;
                adultObjects++;
                object->generation=gc_adult;
            } else {
                MARK(object->generation, 1);
            }
        }
    }

    mutex.unlock();
}

void GarbageCollector::collectAdultObjects() {

    mutex.lock();
    aObjs = 0;

    SharpObject *object;
    for (auto it = heap.begin(); it != heap.end(); ++it) {
        object = *it;

        if(tself->state == THREAD_KILLED) {
            break;
        }

        if(GENERATION(object->generation) == gc_adult) {

            // free object
            if(MARKED(object->generation) && object->refCount == 0) {
                it = sweep(object);
            } else if(MARKED(object->generation) && object->refCount > 0){
                adultObjects--;
                oldObjects++;
                object->generation=gc_old;
            } else
                MARK(object->generation, 1);
        }
    }

    mutex.unlock();
}

void GarbageCollector::collectOldObjects() {

    mutex.lock();
    oObjs = 0;

    SharpObject *object;
    for (auto it = heap.begin(); it != heap.end(); ++it) {
        object = *it;

        if(tself->state == THREAD_KILLED) {
            break;
        }

        if(GENERATION(object->generation) == gc_old) {

            // free object
            if(MARKED(object->generation) && object->refCount == 0) {
                it = sweep(object);
            } else
                MARK(object->generation, 1);
        }
    }

    mutex.unlock();
}

void GarbageCollector::run() {
//    std::random_device rd;
//    std::mt19937 mt(rd());
//    std::uniform_real_distribution<double> dist(1, 10000);

#ifdef SHARP_PROF_
    tself->tprof.init();
#endif

    for(;;) {
        if(tself->suspendPending)
            Thread::suspendSelf();
        if(tself->state == THREAD_KILLED) {
            return;
        }

        if(managedBytes > hbytes)
            hbytes = managedBytes;

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

        while(!tself->suspendPending && !tself->state != THREAD_KILLED
                && !(GC_COLLECT_YOUNG() || GC_COLLECT_ADULT() || GC_COLLECT_OLD()))
        {
#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(999);
#endif
        }
        /**
         * Attempt to collect objects based on the appropriate
         * conditions. This call does not guaruntee that any collections
         * will happen
         */
        collect(GC_CONCURRENT);

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
    self->tself = thread_self;

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

vector<SharpObject *, std::allocator<SharpObject *>>::iterator GarbageCollector::sweep(SharpObject *object) {
    if(object != nullptr) {

        if(object->HEAD != nullptr) {
            managedBytes -= sizeof(double)*object->size;
            std::free(object->HEAD);
        } else if(object->node != nullptr) {
            for(unsigned long i = 0; i < object->size; i++) {
                SharpObject *o = object->node[i].object;

                /**
                 * If the object still has references we just drop it and move on
                 */
                if(o != nullptr && o->refCount <= 1) {
                    sweep(o);
                }
            }

            managedBytes -= sizeof(Object)*object->size;
            std::free(object->node);
        }

        UPDATE_GC(object)
        x++;

        managedBytes -= sizeof(SharpObject)*1;
        std::free(object);

        auto p = std::find(heap.begin(), heap.end(), object);
        if (p != heap.end()) // == myVector.end() means the element was not found
            return heap.erase(p);
    }

    return heap.begin();
}

SharpObject *GarbageCollector::newObject(unsigned long size) {
    if(size==0)
        return nullptr;
    
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
    object->init(size);

    object->HEAD = (double*)__calloc(size, sizeof(double));

    /* track the allocation amount */
    std::lock_guard<recursive_mutex> gd(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(double)*size);
    heap.push_back(object);
    youngObjects++;

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
        object->init(k->fieldCount, k);

        if(k->fieldCount > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*k->fieldCount);
            for(unsigned int i = 0; i < object->size; i++) {
                /**
                 * We want to set the class variables and arrays
                 * to null and initialize the var variables
                 */
                if(k->fields[i].type == VAR && !k->fields[i].isArray) {
                    object->node[i].object = newObject(1);
                    object->node[i].object->refCount++;
                } else {
                    object->node[i].object = nullptr;
                }
            }

        }

        std::lock_guard<recursive_mutex> gd(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*k->fieldCount);
        heap.push_back(object);
        youngObjects++;
        return object;
    }

    return nullptr;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size) {
    if(size==0)
        return nullptr;
    
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
    object->init(size);

    object->node = (Object*)__malloc(sizeof(Object)*size);
    for(unsigned int i = 0; i < object->size; i++)
        object->node[i].object = nullptr;
    
    /* track the allocation amount */
    std::lock_guard<recursive_mutex> gd(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
    heap.push_back(object);
    youngObjects++;

    return object;
}

SharpObject *GarbageCollector::newObjectArray(unsigned long size, ClassObject *k) {
    if(k != nullptr) {
        if(size==0)
            return nullptr;
        
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
        object->init(size, k);

        if(size > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*size);
            for(unsigned int i = 0; i < object->size; i++)
                object->node[i].object = nullptr;
        }

        /* track the allocation amount */
        std::lock_guard<recursive_mutex> gd(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
        heap.push_back(object);
        youngObjects++;

        return object;
    }

    return nullptr;
}

void GarbageCollector::createStringArray(Object *object, native_string& s) {
    if(object != nullptr) {
        *object = newObject(s.len);

        for(unsigned long i = 0; i < s.len; i++) {
            object->object->HEAD[i] = s.chars[i];
        }
    }
}

unsigned long long GarbageCollector::getMemoryLimit() {
    return memoryLimit;
}

unsigned long long GarbageCollector::getManagedMemory() {
    return managedBytes;
}
