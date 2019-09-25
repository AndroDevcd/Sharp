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
#include "../Environment.h"
#include "../Exe.h"
#include "../Manifest.h"
#include <thread>
#include <algorithm>

long long hbytes = 0, freedBytes = 0, freedYoung = 0, freedAdult = 0, freedOld = 0;
long long transferredYoung = 0, transferredAdult = 0, transferredOld = 0;
GarbageCollector *GarbageCollector::self = nullptr;

const int baselineMax = 10;
long long int baseLine[baselineMax];
long long int baselineCount =0, downgradeCount = 0;

void* __malloc(unsigned long long bytes)
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
            gc=true;
            if(GarbageCollector::self != nullptr) {
                GarbageCollector::self->collect(GC_LOW);
                goto alloc_bytes;
            } else
                throw Exception("out of memory");
        }
    } else {
        return ptr;
    }
}
void* __calloc(unsigned long long n, unsigned long long bytes)
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
            if(GarbageCollector::self != nullptr) {
                GarbageCollector::self->collect(GC_LOW);
                goto alloc_bytes;
            } else
                throw Exception("out of memory");
        }
    } else {
        return ptr;
    }
}
void* __realloc(void *ptr, unsigned long long bytes, unsigned long long old)
{
    void* rmap =nullptr;
    bool gc=false;
    alloc_bytes:
    if(GarbageCollector::self != nullptr && !GarbageCollector::self->spaceAvailable(bytes-old))
        goto lowmem;
    rmap=realloc(ptr, bytes);

    if(rmap == nullptr) {
        if(gc) {
            lowmem:
            throw Exception("out of memory");
        } else {
            gc=true;
            if(GarbageCollector::self != nullptr) {
                GarbageCollector::self->collect(GC_LOW);
                goto alloc_bytes;
            } else
                throw Exception("out of memory");
        }
    } else {
        return rmap;
    }
}

void GarbageCollector::initilize() {
    self=(GarbageCollector*)malloc(sizeof(GarbageCollector)*1);
#ifdef WIN32_
    new (&self->mutex) std::mutex();
#endif
#ifdef POSIX_
    new (&self->mutex) std::mutex();
#endif
    self->_Mheap = (SharpObject*)malloc(sizeof(SharpObject)); // HEAD

    if(self->_Mheap==NULL)
        throw Exception("out of memory");
    self->_Mheap->init(0);
    self->tail = self->_Mheap; // set tail to self for later use
    self->heapSize = 0;
    self->managedBytes=0;
    self->memoryLimit = 0;
    self->adultObjects=0;
    self->youngObjects=0;
    self->oldObjects=0;
    self->yObjs=0;
    self->sleep=false;

#ifdef SHARP_PROF_
    self->x = 0;
#endif
    self->aObjs=0;
    self->oObjs=0;
    self->isShutdown=false;
    self->messageQueue.init();
    self->locks.init();
}

void GarbageCollector::releaseObject(Object *object) {
    if(object != nullptr && object->object != nullptr)
    {
        object->object->refCount--;
        switch(GENERATION(object->object->gc_info)) {
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
        self->isShutdown=true;
#ifdef SHARP_PROF_
        cout << "\nsize of object: " << sizeof(SharpObject) << endl;
        cout << "highest memory calculated: " << hbytes << endl;
        cout << "Objects Collected " << self->x << endl;
        cout << "Total managed bytes left " << self->managedBytes << endl;
        cout << "Objects left over young: " << self->youngObjects << " adult: " << self->adultObjects
                                          << " old: " << self->oldObjects << endl;
        unsigned long long noRef = 0;
        SharpObject *p = self->_Mheap;
        while(p) {
            if(p->refCount<=0) {
                noRef++;
                cout << "non referenced object " << self->_sizeof(p) << endl;
            }
            p = p->next;
        }
        cout << "non referenced objects " << noRef << endl;
        //cout << "heap size: " << self->heapSize << endl;
        cout << std::flush << endl;
#endif
        self->locks.free();
        // im no longer freeing memory due to multiple memory references on objects when clearing
        std::free(self->_Mheap);
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
        collectGarbage();

        Thread::resumeAllThreads();
    } else if(policy == GC_EXPLICIT) {
        /**
         * Force collection of both generations
         * We only do the first 2 generations because we want
         * to prevent lag when freeing up memory. the Old generation will
         * always be the largest generation
         */
        collectGarbage();
    } else if(policy == GC_CONCURRENT) {

        /**
         * This should only be called by the GC thread itsself
         */
        if(GC_COLLECT_YOUNG() || GC_COLLECT_ADULT() || GC_COLLECT_OLD()) {        /* 250 objects */
            collectGarbage();
        }
    }

    mutex.lock();
    managedBytes-= freedBytes;
    youngObjects -= freedYoung;
    adultObjects -= freedAdult;
    oldObjects -= freedOld;
    youngObjects += transferredYoung;
    adultObjects += transferredAdult;
    oldObjects += transferredOld;
    freedBytes = 0;
    freedYoung = 0;
    freedAdult = 0;
    transferredYoung = 0;
    transferredAdult = 0;
    transferredOld = 0;
    mutex.unlock();

    /**
     * We want to update the allocation threshold as the program runs for
     * more efficent memory collection
     *
     */
    if(baselineCount == baselineMax) {
        size_t total =0, avg;
        for(long i = 0; i < baselineMax; i++) {
            total+=baseLine[i];
        }
        avg=total/baselineMax;
        baselineCount =0;
        if(avg > memoryThreshold) {
            memoryThreshold = avg; // dynamically update threshold
        } else{
            if(downgradeCount == (baselineMax /2)) {
                memoryThreshold = avg; // downgrade memory due to some free operation
            } else
                downgradeCount++;
        }
    } else {
        baseLine[baselineCount++] = managedBytes;
    }
}

void GarbageCollector::collectGarbage() {

    yObjs = 0;
    SharpObject *object = self->_Mheap->next;
    while(object != NULL) {
        if(tself->state == THREAD_KILLED) {
            break;
        }

        if(GENERATION(object->gc_info) <= gc_old) {
            // free object
            if(MARKED(object->gc_info) && object->refCount == 0) {
                object = sweep(object);
                continue;
            } else if(MARKED(object->gc_info) && object->refCount > 0){
                switch(GENERATION(object->gc_info)) {
                    case gc_young:
                        freedYoung--;
                        transferredAdult++;
                        SET_GENERATION(object->gc_info, gc_adult);
                        break;
                    case gc_adult:
                        freedAdult--;
                        transferredOld++;
                        SET_GENERATION(object->gc_info, gc_old);
                        break;
                    case gc_old:
                        break;
                }
            } else {
                MARK(object->gc_info, 1);
            }
        }

        object = object->next;
    }
}

void GarbageCollector::run() {
//    std::random_device rd;
//    std::mt19937 mt(rd());
//    std::uniform_real_distribution<double> dist(1, 10000);

#ifdef SHARP_PROF_
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

        do {

#ifdef SHARP_PROF_
            if(managedBytes > hbytes)
                hbytes = managedBytes;
#endif

#ifdef WIN32_
            Sleep(1);
#endif
#ifdef POSIX_
            usleep(1*999);
#endif
            if(sleep) sedateSelf();
            if(!messageQueue.empty()) goto message;
            if(GC_LOW_MEM()) break;
        } while(!(GC_COLLECT_MEM() && (GC_COLLECT_YOUNG() || GC_COLLECT_ADULT() || GC_COLLECT_OLD()))
                && !hasSignal(tself->signal, tsig_suspend) && tself->state == THREAD_RUNNING);

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
    self->tself = thread_self;

    try {
        self->run();
    } catch(Exception &e){
        /* Should never happen */
        sendSignal(thread_self->signal, tsig_except, 1);
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

unsigned long long GarbageCollector::_sizeof(SharpObject *object) {
    unsigned long long size =0;
    if(object != nullptr) {

        if(object->type == _stype_var) {
            size += sizeof(double)*object->size;
        } else if(object->type == _stype_struct) {
            for(unsigned long i = 0; i < object->size; i++) {
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

SharpObject* GarbageCollector::sweep(SharpObject *object) {
    if(object != nullptr) {

        if(object->type == _stype_var) {
            freedBytes += sizeof(double)*object->size;
            std::free(object->HEAD); object->HEAD = NULL;
        } else if(object->type == _stype_struct) {
            for(unsigned long i = 0; i < object->size; i++) {
                SharpObject *o = object->node[i].object;

                /**
                 * If the object still has references we just drop it and move on
                 */
                if(o != nullptr && o->refCount <= 1) {
                    sweep(o);
                } else {
                    DEC_REF(o);
                }
            }

            freedBytes += sizeof(Object)*object->size;
            std::free(object->node); object->node = NULL;
        }

//        if(object->mutex != NULL)
//            delete (object->mutex);

        UPDATE_GC(object)

#ifdef SHARP_PROF_
        x++;
#endif

        freedBytes += sizeof(SharpObject);

        SharpObject* tmp = object->next;
        erase(object);
        if(HAS_LOCK(object->gc_info))
            dropLock(object);
        std::free(object);
        return tmp;
    }
    return NULL;
}

SharpObject *GarbageCollector::newObject(int64_t size) {
    if(size==0)
        return nullptr;
    
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
    object->init(size);

    object->HEAD = (double*)__calloc(size, sizeof(double));
    object->type = _stype_var;

    /* track the allocation amount */
    std::lock_guard<recursive_mutex> gd(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(double)*size);
    PUSH(object);
    youngObjects++;

    return object;
}

SharpObject *GarbageCollector::newObject(ClassObject *k) {
    if(k != nullptr) {
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
        object->init(k->fieldCount, k);
        object->type = _stype_struct;

        if(k->fieldCount > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*k->fieldCount);

            for(unsigned int i = 0; i < object->size; i++) {
                /**
                 * We want to set the class variables and arrays
                 * to null and initialize the var variables
                 */
                if(k->fields[i].type == VAR && !k->fields[i].isArray) {
                    object->node[i].object = newObject(1);
                    object->node[i].object->type = _stype_var;
                    object->node[i].object->refCount++;
                } else {
                    object->node[i].object = nullptr;
                }
            }

        }

        std::lock_guard<recursive_mutex> gd(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*k->fieldCount);
        PUSH(object);
        youngObjects++;
        return object;
    }

    return nullptr;
}

SharpObject *GarbageCollector::newObjectArray(int64_t size) {
    if(size==0)
        return nullptr;
    
    SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
    object->init(size);

    object->node = (Object*)__malloc(sizeof(Object)*size);
    object->type = _stype_struct;

    for(unsigned int i = 0; i < object->size; i++)
        object->node[i].object = nullptr;
    
    /* track the allocation amount */
    std::lock_guard<recursive_mutex> gd(mutex);
    managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
    PUSH(object);
    youngObjects++;

    return object;
}

SharpObject *GarbageCollector::newObjectArray(int64_t size, ClassObject *k) {
    if(k != nullptr && size > 0) {
        
        SharpObject *object = (SharpObject*)__malloc(sizeof(SharpObject)*1);
        object->init(size, k);
        object->type = _stype_struct;

        if(size > 0) {
            object->node = (Object*)__malloc(sizeof(Object)*size);

            for(unsigned int i = 0; i < object->size; i++)
                object->node[i].object = nullptr;
        }

        /* track the allocation amount */
        std::lock_guard<recursive_mutex> gd(mutex);
        managedBytes += (sizeof(SharpObject)*1)+(sizeof(Object)*size);
        PUSH(object);
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

void GarbageCollector::erase(SharpObject *p) {
    heapSize--;

    if(p == tail) {
        mutex.lock();
        p->prev->next = p->next;
        if(p == tail) {
            tail = p->prev;
            tail->next = NULL;
        } else  p->next->prev = p->prev;
        mutex.unlock();
        return;
    } else if(p->next != NULL) p->next->prev = p->prev;
    p->prev->next = p->next;
}

void GarbageCollector::realloc(SharpObject *o, size_t sz) {
    if(o != NULL && o->HEAD != NULL) {
        o->HEAD = (double*)__realloc(o->HEAD, sizeof(double)*sz, sizeof(double)*o->size);

        std::lock_guard<recursive_mutex> gd(mutex);
        if(sz < o->size)
            managedBytes -= (sizeof(double)*(o->size-sz));
        else
            managedBytes += (sizeof(double)*(sz-o->size));

        if(sz > o->size)
            std::memset(o->HEAD+o->size, 0, sizeof(double)*(sz-o->size));
        o->size = sz;
    }
}

void GarbageCollector::reallocObject(SharpObject *o, size_t sz) {
    if(o != NULL && o->node != NULL) {
        if(sz < o->size) {
            for(size_t i = sz; i < o->size; i++) {
                if(o->node[i].object != nullptr) {
                    DEC_REF(o->node[i].object)
                }
            }
        }

        o->node = (Object*)__realloc(o->node, sizeof(Object)*sz, sizeof(Object)*o->size);
        std::lock_guard<recursive_mutex> gd(mutex);
        if(sz < o->size)
            managedBytes -= (sizeof(Object)*(o->size-sz));
        else
            managedBytes += (sizeof(Object)*(sz-o->size));


        if(sz > o->size)
            std::memset(o->node+o->size, 0, sizeof(Object)*(sz-o->size));
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
    self->suspended = true;
    self->state = THREAD_SUSPENDED;
    while(sleep) {

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
        if(self->state != THREAD_RUNNING)
            break;
    }

    // we don't want to shoot ourselves in the foot
    if(self->state == THREAD_SUSPENDED)
        self->state = THREAD_RUNNING;
    self->suspended = false;
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
            SET_LOCK(o->gc_info, 1);
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
    if(!sleep && tself->state == THREAD_RUNNING) {
        sleep = true;
        Thread::waitForThreadSuspend(Thread::getThread(gc_threadid));
    }
    mutex.unlock();
}

void GarbageCollector::wake() {
    mutex.lock();
    if(sleep) {
        sleep = false;
        Thread::waitForThread(Thread::getThread(gc_threadid));
    }
    mutex.unlock();
}

int GarbageCollector::selfCollect() {
    if(sleep || tself->state == THREAD_KILLED) {
        mutex.lock();
        collectGarbage();

        managedBytes-= freedBytes;
        youngObjects -= freedYoung;
        adultObjects -= freedAdult;
        oldObjects -= freedOld;
        freedBytes = 0;
        freedYoung = 0;
        freedAdult = 0;
        freedOld = 0;
        mutex.unlock();
        return 0;
    }

    return -1;
}

bool GarbageCollector::isAwake() {
    return !sleep && tself->state == THREAD_RUNNING;
}
