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
#include "../scheduler.h"
#include <thread>
#include <algorithm>

uInt hbytes = 0;
GarbageCollector gc;

const Int MEMORY_POOL_SAMPLE_SIZE = 500;
uInt memoryPoolResults[MEMORY_POOL_SAMPLE_SIZE];
Int samplesReceived =0;

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
    gc._Mheap = (SharpObject*)malloc(sizeof(SharpObject)); // HEAD
    if(gc._Mheap == NULL) throw Exception("out of memory");
    gc._Mheap->init(0, _stype_none);
    gc.tail = gc._Mheap; // set tail to self for later use
}

void GarbageCollector::releaseObject(Object *object) {
    if(object != nullptr && object->object != nullptr)
    {
        DEC_REF(object->object);
        object->object = nullptr;
    }
}

void GarbageCollector::shutdown() {
    GUARD(gc.mutex)
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
        cout << "total time spent collecting us: " << gc.timeSpentCollecting << endl;
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

    if( policy == GC_LOW || policy == GC_EXPLICIT || policy == GC_CONCURRENT ) {
        collectGarbage(true);

        Thread::suspendAllThreads(true);
        /**
         * In order to keep memory utilization low we must shutdown
         * the entire system to perform a colection, this will take
         * on average about 10us to complete
         */
        collectGarbage();
        if(tself->state != THREAD_KILLED)
           Thread::resumeAllThreads(true);
    }

    updateMemoryThreshold();
}

/**
 * We want to update the allocation threshold as the program runs for
 * more efficent memory collection
 *
 */
void GarbageCollector::updateMemoryThreshold() {
    if (samplesReceived == MEMORY_POOL_SAMPLE_SIZE) {
        size_t total = 0, avg;
        for (long i = 0; i < MEMORY_POOL_SAMPLE_SIZE; i++) {
            total += memoryPoolResults[i];
        }

        avg = total / MEMORY_POOL_SAMPLE_SIZE;
        samplesReceived = 0;
        if((avg + (0.15 * memoryLimit)) < memoryLimit)
            memoryThreshold = avg + (0.15 * memoryLimit);
        else memoryThreshold = avg;
    } else {
        memoryPoolResults[samplesReceived++] = managedBytes;
    }
}

void GarbageCollector::collectGarbage(bool markOnly) {
    mutex.lock();
    SharpObject *object = gc._Mheap->next, *prevObj = NULL, *end = tail;
#ifdef SHARP_PROF_
    uInt past = Clock::realTimeInNSecs();
#endif
    mutex.unlock();

    while(object != NULL) {
        if(tself->state == THREAD_KILLED
           || object == end) {
            break;
        }

        if(GENERATION(object->info) <= gc_old) {
            // free object
            if(MARKED(object->info)) {
                if(object->refCount > 0) {
                    MARK(object->info, 0);
                } else if(!markOnly) {
                    object = sweep(object, prevObj);
                    continue;
                }
            }

            if(object->refCount <= 0){
                MARK(object->info, 1);
            } else {
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
                }
            }
        }

        prevObj = object;
        object = object->next;
        if(object != NULL && object->next != NULL && (object->next->next == NULL || object->next->next == end))
            break;
    }

#ifdef SHARP_PROF_
    uInt now = Clock::realTimeInNSecs();
    if(NANO_TOMICRO(now-past) > largestCollectionTime)
        largestCollectionTime = NANO_TOMICRO(now-past);
    timeSpentCollecting += NANO_TOMICRO(now-past);
    collections++;
#endif
}

void GarbageCollector::run() {
#ifdef SHARP_PROF_
    //tself->tprof = new Profiler();
    //tself->tprof->init(2);
#endif

    for(;;) {
        sig:
        if(hasSignal(tself->signal, tsig_suspend))
            Thread::suspendSelf();
        if(tself->state == THREAD_KILLED || hasSignal(tself->signal, tsig_kill)) {
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
            timeSlept += 25;
#endif

            __usleep(25*999);
//            __os_yield();
//#ifdef WIN32_
//            Sleep(25);
//#endif
//#ifdef POSIX_
//            usleep(25*999);
//#endif
            if(state==SLEEPING) sedateSelf();
            if(!messageQueue.empty()) goto message;


            if(GC_COLLECT_MEM()) {
                /**
                 * Attempt to collect objects based on the appropriate
                 * conditions. This call does not guaruntee that any collections
                 * will happen
                 */
                collect(GC_CONCURRENT);
            }

            if(tself->signal)
                goto sig;

        } while(true);
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
    gc.state = RUNNING;
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

SharpObject* GarbageCollector::sweep(SharpObject *object, SharpObject *prevObj) {
    if(object != nullptr) {
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
                if(o != NULL && o->refCount <= 0) MARK(o->info, 1);
            }

            managedBytes -= sizeof(Object)*object->size;
            std::free(object->node); object->node = NULL;
        }

        UPDATE_GC(object)

#ifdef SHARP_PROF_
        x++;
#endif

        heapSize--;
        managedBytes -= sizeof(SharpObject);
        SharpObject* nextObj = object->next;

        GUARD(mutex)
        if(HAS_LOCK(object->info))
            dropLock(object);

        prevObj->next = object->next;
        std::free(object);
        return nextObj;
    }
    return NULL;
}

SharpObject *GarbageCollector::newObject(int64_t size, unsigned short ntype) {
    if(size<=0)
        return nullptr;

    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject));
    object->init(size, _stype_var);

    object->HEAD = (double*)__calloc(size, sizeof(double));
    object->ntype = ntype % 9;
    object->array = size > 1;

    /* track the allocation amount */
    GUARD(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(double)*size);
    PUSH(object);
    youngObjects++;

    return object;
}

SharpObject *GarbageCollector::newObjectUnsafe(int64_t size, unsigned short ntype) {
    if(size<=0)
        return nullptr;

    SharpObject *object = (SharpObject*)malloc(sizeof(SharpObject));
    if(object != NULL) {
        object->init(size, _stype_var);

        object->HEAD = (double *) calloc(size, sizeof(double));
        object->array = size > 1;

        if(object->HEAD != NULL) {
            object->ntype = ntype % 9;
            SET_TYPE(object->info, _stype_var);

            /* track the allocation amount */
            GUARD(mutex);
            managedBytes += (sizeof(SharpObject) * 1) + (sizeof(double) * size);
            PUSH(object);
            youngObjects++;
        } else {
            std::free(object);
            return NULL;
        }
    }

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k, bool staticInit, bool autoInit) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject));
        uint32_t size = staticInit ? k->staticFields : k->instanceFields;

        object->init(size, k);
        if(size > 0) {
            object->node = (Object*)__calloc(size, sizeof(struct Object));
            if(autoInit) {
                uInt fieldAddress = staticInit ? k->instanceFields : 0;

                for (unsigned int i = 0; i < object->size; i++) {
                    /**
                     * We want to set the class variables and arrays
                     * to null and initialize the var variables
                     */
                    if (k->fields[fieldAddress].type <= VAR && !k->fields[fieldAddress].isArray) {
                        if (!staticInit || (staticInit && IS_STATIC(k->fields[fieldAddress].flags))) {
                            object->node[i] = newObject(1, k->type < FNPTR ? k->type : NTYPE_VAR);
                        }
                    }

                    fieldAddress++;
                }
            }

        }

        GUARD(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
        PUSH(object);
        youngObjects++;
        return object;
    }

    return nullptr;
}

SharpObject *GarbageCollector::newObjectUnsafe(ClassObject *k, bool staticInit) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)malloc(sizeof(SharpObject));
        if(object != NULL) {
            uint32_t size = staticInit ? k->staticFields : k->instanceFields;

            object->init(size, k);
            if (size > 0) {
                object->node = (Object *) calloc(size, sizeof(struct Object));
                if (object->node != NULL) {
                    uInt fieldAddress = staticInit ? k->instanceFields : 0;

                    for (unsigned int i = 0; i < object->size; i++) {
                        /**
                         * We want to set the class variables and arrays
                         * to null and initialize the var variables
                         */
                        if (k->fields[fieldAddress].type <= VAR && !k->fields[fieldAddress].isArray) {
                            if (!staticInit || (staticInit && IS_STATIC(k->fields[fieldAddress].flags))) {
                                object->node[i] = newObjectUnsafe(1, k->type < FNPTR ? k->type : NTYPE_VAR);
                            }
                        }

                        fieldAddress++;
                    }

                    GUARD(mutex);
                    managedBytes += (sizeof(SharpObject) * 1) + (sizeof(Object) * size);
                    PUSH(object);
                    youngObjects++;
                } else {
                    free(object);
                    return NULL;
                }

            }
            return object;
        }
    }

    return nullptr;
}

SharpObject *GarbageCollector::newObjectArray(int64_t size) {
    if(size<=0)
        return nullptr;

    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject));

    object->init(size, _stype_struct);
    object->node = (Object*)__calloc(size, sizeof(struct Object)*1);
    object->array = 1;

    /* track the allocation amount */
    GUARD(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
    PUSH(object);
    youngObjects++;

    return object;
}

SharpObject *GarbageCollector::newObjectArray(int64_t size, ClassObject *k) {
    if(k != nullptr && size > 0) {

        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
        object->init(size, k);

        if(size > 0)
            object->node = (Object*)__calloc(size, sizeof(struct Object));
        object->array = 1;

        /* track the allocation amount */
        GUARD(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
        PUSH(object);
        youngObjects++;

        return object;
    }

    return nullptr;
}

void GarbageCollector::createStringArray(Object *object, runtime::String& str) {
    if(object != nullptr) {
        *object = newObject(str.len, _INT8);

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

        o->node = (Object*)__realloc(o->node, sizeof(Object)*sz, sizeof(Object)*o->size);
        GUARD(mutex);
        if(sz < o->size)
            managedBytes -= (sizeof(Object)*(o->size-sz));
        else
            managedBytes += (sizeof(Object)*(sz-o->size));


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
        __os_yield();
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

SharpObject *GarbageCollector::getObjectAt(uInt index) {
    gc.mutex.lock();
    SharpObject *object = _Mheap->next, *prevObj = NULL, *end = tail;

    while(object != NULL) {
        if(object == end || --index == 0) {
            break;
        }

        if(IS_CLASS(object->info)) {
            vm.classes[CLASS(object->info)].gcRefs++;
        }

        object = object->next;
    }
    gc.mutex.unlock();
    return object;
}

void GarbageCollector::printClassRefStatus() {
    gc.mutex.lock();
    SharpObject *object = _Mheap->next, *prevObj = NULL, *end = tail;

    for(Int i = 0; i < vm.manifest.classes; i++) {
        vm.classes[i].gcRefs = 0;
    }

    while(object != NULL) {
        if(object == end) {
            break;
        }

        if(IS_CLASS(object->info)) {
            vm.classes[CLASS(object->info)].gcRefs++;
        }

        object = object->next;
    }

    cout << "\n\n Class gc ref count:\n";
    for(Int i = 0; i < vm.manifest.classes; i++) {
        cout << vm.classes[i].fullName.str() << " : " << vm.classes[i].gcRefs << endl;
    }
    cout << "\n\n";
    gc.mutex.unlock();

}

recursive_mutex lockCheckMutex;
bool GarbageCollector::lock(SharpObject *o, Thread* thread) {
    if(o) {
        mutex_t *mut;
        mutex.lock();
        long long idx = locks.indexof(isLocker, o);
        if(idx != -1)
            mut = locks.get(idx);
        else {
            managedBytes += sizeof(mutex_t)+sizeof(recursive_mutex);
            mut = new mutex_t(o, new recursive_mutex());
            locks.add(mut);
            SET_LOCK(o->info, 1);
        }
        mutex.unlock();

        thread->this_fiber->locking =true;

        retry:
        long maxSpin = 100000;
        long spins = 0;
        if(mut->fiberid != thread->this_fiber->id) {
            if (mut->threadid == thread->id) {
                auto fib = fiber::getFiber(mut->fiberid);
                auto bound = fib->boundThread;

                if(bound && bound->id == thread->id)
                  thread->next_fiber = fib;
                thread->this_fiber->delay(2000);
                return false;
            }

            while (mut->fiberid != -1) {
                if (spins++ == maxSpin) {
                    spins = 0;
                    if (thread->contextSwitching) {
                        return false;
                    }
                    else if (hasSignal(thread->signal, tsig_kill) || thread->state == THREAD_KILLED) {
                        return true;
                    }

                    __usleep(100);
                } else if (hasSignal(thread->signal, tsig_context_switch)) {
                    if (!(hasSignal(thread->signal, tsig_suspend) || hasSignal(thread->signal, tsig_except))) {
                        thread->try_context_switch();

                        if (thread->contextSwitching) {
                            thread->this_fiber->delay(2000);
                            return false;
                        } else {
                            thread->enableContextSwitch(false);
                        }
                    }
                }
            }
        }

        lockCheckMutex.lock();
        if(mut->fiberid != thread->this_fiber->id) {
            if (mut->fiberid != -1) {
                lockCheckMutex.unlock();
                goto retry;
            }
        }

        mut->fiberid = thread->this_fiber->id;
        thread->this_fiber->locking =false;
        mut->threadid=thread->id;
        mut->lockedCount++;
        lockCheckMutex.unlock();
    }

    return true;
}

void GarbageCollector::unlock(SharpObject *o) {
    if(o) {
        mutex_t *mut=0;
        mutex.lock();
        long long idx = locks.indexof(isLocker, o);
        if(idx != -1)
            mut = locks.get(idx);
        mutex.unlock();
        if(mut) {

            lockCheckMutex.lock();
            if (mut->fiberid != -1) {
                mut->lockedCount--;
                if(mut->lockedCount <= 0) {
                    mut->fiberid = -1;
                    mut->threadid = -1;
                    mut->lockedCount=0;
                }
            }

            lockCheckMutex.unlock();

        }
    }
}

void GarbageCollector::reconcileLocks(Thread* thread) {
    if(thread== NULL)
        return;

    mutex.lock();
    lockCheckMutex.lock();
    for(long long i = 0; i < locks.size(); i++) {
        mutex_t *mut = locks.get(i);
        if(mut->threadid==thread->id) {
            mut->threadid = -1;
            mut->lockedCount = 0;
        }

    }
    lockCheckMutex.unlock();
    mutex.unlock();
}

void GarbageCollector::reconcileLocks(fiber* fib) {
    if(fib== NULL)
        return;

    mutex.lock();
    lockCheckMutex.lock();
    for(long long i = 0; i < locks.size(); i++) {
        mutex_t *mut = locks.get(i);
        if(mut->fiberid==fib->id) {
            mut->fiberid = -1;
            mut->threadid = -1;
            mut->lockedCount = 0;
        }

    }
    lockCheckMutex.unlock();
    mutex.unlock();
}

void GarbageCollector::dropLock(SharpObject *o) {
    if(o) {
        mutex.lock();
        long long idx = locks.indexof(isLocker, o);
        if(idx >= 0) {
            mutex_t *mut = locks.get(idx);
            if(mut->threadid!= -1) {
                mut->threadid = -1;
                mut->lockedCount = 0;
            }
            managedBytes -= sizeof(mutex_t) + sizeof(recursive_mutex);
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
        collect(GC_EXPLICIT);
        return 0;
    }

    return -1;
}

bool GarbageCollector::isAwake() {
    return gc.state == RUNNING && tself->state == THREAD_RUNNING;
}
