//
// Created by BraxtonN on 2/11/2018.
//

#include <random>
#include "GarbageCollector.h"
#include "../symbols/Object.h"
#include "../Thread.h"
#include "../symbols/Field.h"
#include "../../util/time.h"
#include "../../util/KeyPair.h"
#include "../Exe.h"
#include "../Manifest.h"
#include "../VirtualMachine.h"
#include <thread>
#include <algorithm>

uInt hbytes = 0;
GarbageCollector gc;

const Int MEMORY_POOL_SAMPLE_SIZE = 10;
const Int MAX_DOWNGRADE_ATTEMPTS = 3;
uInt memoryPoolResults[MEMORY_POOL_SAMPLE_SIZE];
Int samplesReceived =0, downgradeAttempts = 0;

void* __malloc(uInt bytes)
{
    void* ptr =nullptr;
    bool tried=false;
    alloc_bytes:
    if(gc.state >= RUNNING && !gc.spaceAvailable(bytes))
        goto lowmem;
    ptr=malloc(bytes);

    if(gc.state >= RUNNING && ptr == nullptr) {
        if(tried) {
            lowmem:
            throw Exception(vm.OutOfMemoryExcept, "out of memory");
        } else {
            tried=true;
            if(vm.state == VM_RUNNING && gc.state >= RUNNING) {
                gc.collect(GC_LOW);
                goto alloc_bytes;
            } else
                throw Exception(vm.OutOfMemoryExcept, "out of memory");
        }
    } else {
        return ptr;
    }
}
void* __calloc(uInt n, uInt bytes)
{
    void* ptr =nullptr;
    bool tried=false;
    alloc_bytes:
    if(gc.state >= RUNNING && !gc.spaceAvailable(n*bytes))
        goto lowmem;
    ptr=calloc(n, bytes);

    if(ptr == nullptr) {
        if(tried) {
            lowmem:
            throw Exception(vm.OutOfMemoryExcept, "out of memory");
        } else {
            tried=true;
            if(vm.state == VM_RUNNING && gc.state >= RUNNING) {
                gc.collect(GC_LOW);
                goto alloc_bytes;
            } else
                throw Exception(vm.OutOfMemoryExcept, "out of memory");
        }
    } else {
        return ptr;
    }
}
void* __realloc(void *ptr, uInt bytes, uInt old)
{
    void* rmap =nullptr;
    bool tried=false;
    alloc_bytes:
    if(gc.state >= RUNNING && !gc.spaceAvailable(bytes-old))
        goto lowmem;
    rmap=realloc(ptr, bytes);

    if(rmap == nullptr) {
        if(tried) {
            lowmem:
            throw Exception(vm.OutOfMemoryExcept, "out of memory");
        } else {
            tried=true;
            if(vm.state == VM_RUNNING && gc.state >= RUNNING) {
                gc.collect(GC_LOW);
                goto alloc_bytes;
            } else
                throw Exception(vm.OutOfMemoryExcept, "out of memory");
        }
    } else {
        return rmap;
    }
}

void GarbageCollector::initilize() {
    gc._Mheap = (SharpObject*)__malloc(sizeof(SharpObject)); // HEAD
    if(gc._Mheap == NULL) throw Exception("out of memory");
    gc._Mheap->init(0, _stype_none);
    gc.tail = gc._Mheap; // set tail to self for later use
}

void GarbageCollector::releaseObject(Object *object) {
    if(object != nullptr && object->object != nullptr)
    {
        object->object->refCount--;
        switch(GENERATION(object->object->info)) {
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
    if(gc.state != SHUTDOWN) {
        gc.state=SHUTDOWN;
#ifdef SHARP_PROF_
        cout << "\nsize of object: " << sizeof(SharpObject) << endl;
        cout << "highest memory calculated: " << hbytes << endl;
        cout << "Objects Collected " << gc.x << endl;
        cout << "Total managed bytes left " << gc.managedBytes << endl;
        cout << "Objects left over young: " << gc.youngObjects << " adult: " << gc.adultObjects
                                          << " old: " << gc.oldObjects << endl;
        cout << "largest collection " << gc.largestCollectionTime << endl;
        cout << "total collections " << gc.collections << endl;
        cout << "total time spent collecting " << gc.timeSpentCollecting << endl;
        cout << "total time spent sleeping " << gc.timeSlept << endl;
        cout << "heap size: " << gc.heapSize << endl;
        cout << std::flush << endl;
#endif
        gc.locks.free();
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
    if(isShutdown())
        return;

    if(policy == GC_LOW) {
        Thread::suspendAllThreads();

        /**
         * To attempt to approve a large memory request we want to take the
         * worst case scenareo route to try to fuffil the memory request. To
         * avoid lags in the application big memory requests should not be
         * performed often
         */
        collectGarbage();

        Thread::resumeAllThreads();
    } else if(policy == GC_EXPLICIT) {
        /**
         * Force collection of both generations
         * We only do the first 2 generations because we want
         * to prevent lag when freeing up memory. the Old generation will
         * always be the largest generation
         */
        if(GC_COLLECT_YOUNG() || GC_COLLECT_ADULT() || GC_COLLECT_OLD()) {
            collectGarbage();
        }
    } else if(policy == GC_CONCURRENT) {

        /**
         * This should only be called by the GC thread itsself
         */
        collectGarbage();
    }

    updateMemoryThreshold();
}

/**
 * We want to update the allocation threshold as the program runs for
 * more efficent memory collection
 *
 */
void GarbageCollector::updateMemoryThreshold() {
    if(GC_LOW_MEM()) {
        memoryThreshold = (0.85 * memoryLimit);
    } else {
        if (samplesReceived == MEMORY_POOL_SAMPLE_SIZE) {
            size_t total = 0, avg;
            for (long i = 0; i < MEMORY_POOL_SAMPLE_SIZE; i++) {
                total += memoryPoolResults[i];
            }

            avg = total / MEMORY_POOL_SAMPLE_SIZE;
            samplesReceived = 0;
            if (avg > memoryThreshold) {
                memoryThreshold = avg; // dynamically update threshold
            } else {
                if (downgradeAttempts++ == MAX_DOWNGRADE_ATTEMPTS) {
                    memoryThreshold = avg; // downgrade memory due to some free operation
                    downgradeAttempts = 0;
                }
            }
        } else {
            memoryPoolResults[samplesReceived++] = managedBytes;
        }
    }
}

void GarbageCollector::collectGarbage() {
    mutex.lock();
    yObjs = 0; aObjs = 0; oObjs = 0;
    SharpObject *object = gc._Mheap->next, *prevObj = gc._Mheap, *cachedTail = NULL;
    cachedTail = tail;
#ifdef SHARP_PROF_
    uInt past = Clock::realTimeInNSecs();
#endif
    mutex.unlock();

    while(object != NULL) {
        if(tself->state == THREAD_KILLED
            || object == cachedTail) {
            break;
        }

        if(GENERATION(object->info) <= gc_old) {
            // free object
            if(MARKED(object->info) && object->refCount == 0) {
                object = sweep(object, prevObj, cachedTail);
                continue;
            } else if(MARKED(object->info) && object->refCount > 0){
                switch(GENERATION(object->info)) {
                    case gc_young:
                        youngObjects--;
                        adultObjects++;
                        SET_GENERATION(object->info, gc_adult);
                        break;
                    case gc_adult:
                        adultObjects--;
                        oldObjects++;
                        SET_GENERATION(object->info, gc_old);
                        break;
                    case gc_old:
                        break;
                }
            } else {
                MARK(object->info, 1);
            }
        }

        prevObj = object;
        object = object->next;
    }

#ifdef SHARP_PROF_
    uInt now = Clock::realTimeInNSecs();
    if(NANO_TOMILL(now-past) > largestCollectionTime)
        largestCollectionTime = NANO_TOMILL(now-past);
    timeSpentCollecting += NANO_TOMILL(now-past);
    collections++;
#endif
}

void GarbageCollector::run() {
#ifdef SHARP_PROF_
    tself->tprof = new Profiler();
    tself->tprof->init(2);
#endif

    for(;;) {
        if(hasSignal(tself->signal, tsig_suspend))
            Thread::suspendSelf();
        if(tself->state == THREAD_KILLED) {
            return;
        }

        message:
        if(!messageQueue.empty()) {
            CollectionPolicy policy;
            {
                GUARD(mutex);
                policy = messageQueue.last();
                messageQueue.pop_back();
            }

            /**
             * We only want to run a concurrent collection
             * in the GC thread itsself
             */
            if(policy != GC_CONCURRENT)
                collect(policy);
        }

        do {

#ifdef SHARP_PROF_
            if(managedBytes > hbytes)
                hbytes = managedBytes;
            timeSlept += 15;
#endif

            __os_yield();
#ifdef WIN32_
            Sleep(15);
#endif
#ifdef POSIX_
            usleep(15*999);
#endif
            if(state==SLEEPING) sedateSelf();
            if(!messageQueue.empty()) goto message;
        } while(!(GC_COLLECT_MEM() && (GC_COLLECT_YOUNG() || GC_COLLECT_ADULT() || GC_COLLECT_OLD()))
                && !tself->signal);

        if(tself->state == THREAD_KILLED)
            return;

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
    gc.tself = thread_self;
    Thread::setPriority(thread_self, THREAD_PRIORITY_LOW);

    try {
        gc.run();
    } catch(Exception &e){
        /* Should never happen */
        sendSignal(thread_self->signal, tsig_except, 1);
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
    GUARD(mutex);
    messageQueue.push_back(message);
}

double GarbageCollector::_sizeof(SharpObject *object) {
    double size =0;
    if(object != nullptr) {

        if(TYPE(object->info) == _stype_var) {
            size += sizeof(double)*object->size;
        } else if(TYPE(object->info) == _stype_struct) {
            for(uInt i = 0; i < object->size; i++) {
                SharpObject *o = object->node[i].object;

                /**
                 * If the object still has references we just drop it and move on
                 */
                if(o != nullptr) {
                    size += _sizeof(o);
                }
            }

            size += sizeof(Object)*object->size;
        }

        size += sizeof(SharpObject);
    }

    return size;
}

SharpObject* GarbageCollector::sweep(SharpObject *object, SharpObject *prevObj, SharpObject *tail) {
    if(object != nullptr) {

        sharp_type st = (sharp_type)TYPE(object->info);
        ClassObject *klass = &vm.classes[CLASS(object->info) % vm.manifest.classes];
        int gen = GENERATION(object->info);

        if(TYPE(object->info) == _stype_var) {
            managedBytes -= sizeof(double)*object->size;
            std::free(object->HEAD); object->HEAD = NULL;
        } else if(TYPE(object->info) == _stype_struct) {
            for(unsigned long i = 0; i < object->size; i++) {
                SharpObject *o = object->node[i].object;

                /**
                 * If the object still has references we just drop it and move on
                 */
                DEC_REF(o);
            }

            managedBytes -= sizeof(Object)*object->size;
            std::free(object->node); object->node = NULL;
        }

//        if(object->mutex != NULL)
//            delete (object->mutex);

        UPDATE_GC(object)

#ifdef SHARP_PROF_
        x++;
#endif

        managedBytes -= sizeof(SharpObject);

        SharpObject* nextObj = object->next;
        erase(object, prevObj, tail);
        std::free(object);
        return nextObj;
    }
    return NULL;
}

SharpObject *GarbageCollector::newObject(int64_t size) {
    if(size<=0)
        return nullptr;
    
    SharpObject *object = (SharpObject*)__malloc(sizeof(struct SharpObject));
    new (object) SharpObject();
    object->init(size, _stype_var);

    object->HEAD = (double*)__calloc(size, sizeof(double));
    SET_TYPE(object->info, _stype_var);

    /* track the allocation amount */
    GUARD(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(double)*size);
    PUSH(object);
    youngObjects++;
    heapSize++;

    return object;
}

SharpObject *GarbageCollector::newObjectUnsafe(int64_t size) {
    if(size<=0)
        return nullptr;

    SharpObject *object = (SharpObject*)malloc(sizeof(struct SharpObject));
    new (object) SharpObject();
    if(object != NULL) {
        object->init(size, _stype_var);

        object->HEAD = (double *) calloc(size, sizeof(double));
        if(object->HEAD != NULL) {
            SET_TYPE(object->info, _stype_var);

            /* track the allocation amount */
            GUARD(mutex);
            managedBytes += (sizeof(struct SharpObject) * 1) + (sizeof(double) * size);
            PUSH(object);
            youngObjects++;
            heapSize++;
        } else {
            std::free(object);
            return NULL;
        }
    }

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k, bool staticInit) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(struct SharpObject));
        new (object) SharpObject();
        uint32_t size = staticInit ? k->staticFields : k->instanceFields;

        object->init(size, k);
        if(size > 0) {
            object->node = (Object*)__calloc(size, sizeof(struct Object));
            uInt fieldAddress =  staticInit ? k->instanceFields : 0;

            for(unsigned int i = 0; i < object->size; i++) {
                /**
                 * We want to set the class variables and arrays
                 * to null and initialize the var variables
                 */
                if(k->fields[fieldAddress].type <= VAR && !k->fields[fieldAddress].isArray) {
                    if(!staticInit || (staticInit && IS_STATIC(k->fields[fieldAddress].flags))) {
                        object->node[i] = newObject(1);
                    }
                }

                fieldAddress++;
            }

        }

        GUARD(mutex);
        managedBytes += (sizeof(struct SharpObject)*1)+(sizeof(struct Object)*size);
        PUSH(object);
        youngObjects++;
        heapSize++;
        return object;
    }

    return nullptr;
}

SharpObject *GarbageCollector::newObjectArray(int64_t size) {
    if(size<=0)
        return nullptr;
    
    SharpObject *object = (SharpObject*)__malloc(sizeof(struct SharpObject));
    new (object) SharpObject();

    object->init(size, _stype_struct);
    object->node = (Object*)__calloc(size, sizeof(struct Object)*1);
    
    /* track the allocation amount */
    GUARD(mutex);
    managedBytes += (sizeof(struct SharpObject)*1)+(sizeof(struct Object)*size);
    PUSH(object);
    youngObjects++;
    heapSize++;

    return object;
}

SharpObject *GarbageCollector::newObjectArray(int64_t size, ClassObject *k) {
    if(k != nullptr && size > 0) {
        
        SharpObject *object = (SharpObject*)__malloc(sizeof(struct SharpObject)*1);
        new (object) SharpObject();
        object->init(size, k);

        if(size > 0)
            object->node = (Object*)__calloc(size, sizeof(struct Object));

        /* track the allocation amount */
        GUARD(mutex);
        managedBytes += (sizeof(struct SharpObject)*1)+(sizeof(struct Object)*size);
        PUSH(object);
        youngObjects++;
        heapSize++;

        return object;
    }

    return nullptr;
}

void GarbageCollector::createStringArray(Object *object, runtime::String& str) {
    if(object != nullptr) {
        *object = newObject(str.len);

        if(object->object != NULL) {
            double *array = object->object->HEAD;
            for (unsigned long i = 0; i < str.len; i++) {
                *array = str.chars[i];
                array++;
            }
        }
    }
}

uInt GarbageCollector::getMemoryLimit() {
    return memoryLimit;
}

uInt GarbageCollector::getManagedMemory() {
    return managedBytes;
}

SharpObject* GarbageCollector::erase(SharpObject *freedObj, SharpObject *prevObj, SharpObject *tail) {
    heapSize--;

    if(HAS_LOCK(freedObj->info))
        dropLock(freedObj);

    if(tail == freedObj){
        GUARD(mutex);
        prevObj->next = freedObj->next;
    } else prevObj->next = freedObj->next;
    return prevObj->next;
}

void GarbageCollector::realloc(SharpObject *o, size_t sz) {
    if(o != NULL && TYPE(o->info) == _stype_var) {
        o->HEAD = (double*)__realloc(o->HEAD, sizeof(double)*sz, sizeof(double)*o->size);

        GUARD(mutex);
        if(sz < o->size)
            managedBytes -= (sizeof(double)*(o->size-sz));
        else
            managedBytes += (sizeof(double)*(sz-o->size));

        if(sz > o->size) {
            for(Int i = o->size; i < sz; i++) {
                o->HEAD[i] = 0;
            }
        }
        o->size = sz;
    }
}

void GarbageCollector::reallocObject(SharpObject *o, size_t sz) {
    if(o != NULL && TYPE(o->info) == _stype_struct && o->node != NULL) {
        if(sz < o->size) {
            for(size_t i = sz; i < o->size; i++) {
                if(o->node[i].object != nullptr) {
                    DEC_REF(o->node[i].object)
                }
            }
        }

        o->node = (Object*)__realloc(o->node, sizeof(struct Object)*sz, sizeof(struct Object)*o->size);
        GUARD(mutex);
        if(sz < o->size)
            managedBytes -= (sizeof(struct Object)*(o->size-sz));
        else
            managedBytes += (sizeof(struct Object)*(sz-o->size));


        if(sz > o->size) {
            for(Int i = o->size; i < sz; i++) {
                o->node[i].object = NULL;
            }
        }

        o->size = sz;
    }
}

void GarbageCollector::kill() {
    mutex.lock();
    if(tself->state == THREAD_RUNNING) {
        tself->state = THREAD_KILLED;
        sendSignal(tself->signal, tsig_kill, 1);
        Thread::waitForThreadExit(tself);
    }

    mutex.unlock();
}

void GarbageCollector::sedateSelf() {
    Thread* self = thread_self;
    gc.state = SLEEPING;
    tself->state = THREAD_SUSPENDED;
    while(gc.state == SLEEPING) {

#ifdef SHARP_PROF_
        if(managedBytes > hbytes)
                hbytes = managedBytes;
#endif

#ifdef WIN32_
        Sleep(30);
#endif
#ifdef POSIX_
        usleep(30*999);
#endif
        if(tself->state != THREAD_RUNNING)
            break;
    }

    // we don't want to shoot ourselves in the foot
    if(tself->state == THREAD_SUSPENDED)
        tself->state = THREAD_RUNNING;
    gc.state = RUNNING;
}

bool isLocker(void *o, mutex_t* mut) {
    return (SharpObject*)o == mut->object;
}

void GarbageCollector::lock(SharpObject *o, Thread* thread) {
    if(o) {
        mutex_t *mut;
        mutex.lock();
        long long idx = locks.indexof(isLocker, o);
        if(idx != -1)
            mut = locks.get(idx);
        else {
            managedBytes += sizeof(mutex_t)+sizeof(recursive_mutex);
            mut = new mutex_t(o, new recursive_mutex(), -1);
            locks.add(mut);
            SET_LOCK(o->info, 1);
        }
        mutex.unlock();
        if(mut->threadid != thread->id) {
            mut->mutex->lock();
            mut->threadid = thread->id;
        }
    }
}

void GarbageCollector::unlock(SharpObject *o, Thread* thread) {
    if(o) {
        mutex_t *mut=0;
        mutex.lock();
        long long idx = locks.indexof(isLocker, o);
        if(idx != -1)
            mut = locks.get(idx);
        mutex.unlock();
        if(mut && mut->threadid==thread->id) {
            mut->threadid = -1;
            mut->mutex->unlock();
        } 
    }
}

void GarbageCollector::reconcileLocks(Thread* thread) {

    mutex.lock();
    for(long long i = 0; i < locks.size(); i++) {
        mutex_t *mut = locks.get(i);
        if(mut->threadid==thread->id) {
            mut->threadid = -1;
            mut->mutex->unlock();
        }
    }
    mutex.unlock();
}

void GarbageCollector::dropLock(SharpObject *o) {
    if(o) {
        mutex.lock();
        long long idx = locks.indexof(isLocker, o);
        if(idx != -1) {
            mutex_t *mut = locks.get(idx);
            if(mut->threadid!= -1)
                mut->mutex->unlock();
            managedBytes -= sizeof(mutex_t) + sizeof(recursive_mutex);
            delete mut->mutex;
            delete mut;
            locks.remove(idx);
        }
        mutex.unlock();
    }
}

void GarbageCollector::sedate() {
    mutex.lock();
    if(gc.state != SLEEPING && tself->state == THREAD_RUNNING) {
        gc.state = SLEEPING;
        Thread::suspendAndWait(Thread::getThread(gc_threadid));
    }
    mutex.unlock();
}

void GarbageCollector::wake() {
    mutex.lock();
    if(gc.state == SLEEPING) {
        gc.state = RUNNING;
        Thread::waitForThread(Thread::getThread(gc_threadid));
    }
    mutex.unlock();
}

int GarbageCollector::selfCollect() {
    if(gc.state == SLEEPING || tself->state == THREAD_KILLED) {
        mutex.lock();
        collectGarbage();
        mutex.unlock();
        return 0;
    }

    return -1;
}

bool GarbageCollector::isAwake() {
    return gc.state == RUNNING && tself->state == THREAD_RUNNING;
}
