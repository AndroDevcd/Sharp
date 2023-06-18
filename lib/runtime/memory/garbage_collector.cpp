//
// Created by bknun on 9/16/2022.
//

#include "garbage_collector.h"
#include "../virtual_machine.h"
#include "../../util/time.h"
#include "../multitasking/thread/thread_controller.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../../core/thread_state.h"
#include "../error/vm_exception.h"
#include "../multitasking/fiber/fiber.h"
#include "sharp_object.h"

garbage_collector gc;
recursive_mutex gc_lock;

void reserve_bytes(size_t bytes, bool unsafe = false) {

    if(unsafe) {
        guard_mutex(gc_lock)
        gc.managedBytes += bytes;
    } else {
        if ((bytes + gc.managedBytes) >= gc.memoryLimit) {
            auto gcThread = get_thread(gc_threadid);
            suspend_and_wait(gcThread, true);
            gc_collect(policy_full_sweep);
        }

        if ((bytes + gc.managedBytes) < gc.memoryLimit) {
            guard_mutex(gc_lock)
            gc.managedBytes += bytes;
        } else {
            throw vm_exception(vm.out_of_memory_except, "out of memory");
        }
    }
}

void set_memory_limit(Int limit) {
    gc.memoryLimit = limit;
}

void set_memory_threshold(Int threshold) {
    gc.memoryThreshold = threshold;
}

void release_bytes(size_t bytes) {
    guard_mutex(gc_lock)

    if(gc.managedBytes - bytes >= 0) {
        gc.managedBytes -= bytes;
    } else {
        gc.managedBytes = 0;
    }
}

void push_object(sharp_object *o) {
    guard_mutex(gc_lock)
    auto old = gc.yMemHead;
    o->next = old;
    gc.yMemHead = o;
}

bool is_mutex(void *o, node<fib_mutex*> *node) {
    return (sharp_object*)o == node->data->o;
}

bool mutex_deleted(void *o, node<fib_mutex*> *node) {
    if(is_mutex(o, node)) {
        delete node->data;
        return true;
    }

    return false;
}

fib_mutex* get_mutex(sharp_object *o) {
    auto node = gc.f_locks.node_at(o, is_mutex);
    return node ? node->data : nullptr;
}

void remove_mutex(sharp_object *o) {
    guard_mutex(gc_lock)
    gc.f_locks.delete_at(o, mutex_deleted);
}

fib_mutex* create_mutex(sharp_object *o) {
    guard_mutex(gc_lock)
    auto mut = get_mutex(o);

    if(mut) return mut;
    else {
        mut = new fib_mutex();
        mut->id = -1;
        mut->o = o;
        SET_LOCK(o->info, 1);
        gc.f_locks.createnode(mut);
        return mut;
    }
}

void release_all_mutexes(fiber *task) {
    guard_mutex(gc_lock)
    gc.f_locks.foreach(task, [](void *data, node<fib_mutex*> *item) {
        auto task = (fiber *)data;

        if(item->data->id == task->id) {
            item->data->id = -1;
        }
    });
}

void age_mem(sharp_object *prev, sharp_object *ager, sharp_object **newHeap)
{
    if(prev != nullptr && ager != nullptr) {
        guard_mutex(gc_lock)
        prev->next = ager->next;
        ager->next = *newHeap;
        *newHeap = ager;
    }
}

void mark_memory(sharp_object *heap) {
    sharp_object *prev = nullptr;
    if(heap != nullptr) {
        guard_mutex(gc_lock)
        prev = heap;
        heap = heap->next;
    }

    while(heap != nullptr)
    {
        CHECK_STATE

        if(GENERATION(heap->info) <= gc_old) {
            if(heap->refCount == 0) {
                MARK(heap->info, 1);
            } else if(heap->refCount <= 0) {

                auto next = heap->next;
                switch (GENERATION(heap->info)) {

                    case gc_young: {
                        age_mem(prev, heap, &gc.aMemHead);
                        break;
                    }

                    case gc_adult: {
                        age_mem(prev, heap, &gc.oMemHead);
                        break;
                    }

                    default:
                        break;
                }

                heap = next;
                continue;
            }
        }

        prev = heap;
        heap = heap->next;
    }
}

void sweep(sharp_object *prev, sharp_object *obj) {
    if(obj != nullptr && prev != nullptr)
    {
        if(obj->type <= type_var) {
            release_bytes(sizeof(long double) * obj->size);
            std::free(obj->HEAD);
        } else if(obj->node != nullptr) {
            for(Int i = 0; i < obj->size; i++) {
                auto child = obj->node[i].o;

                if(child != nullptr) {
                    dec_ref(child)
                    if(child->refCount <= 0) MARK(child->info, 1);
                }
            }

            release_bytes(sizeof(object) * obj->size);
            std::free(obj->node);
        }

        release_bytes(sizeof(sharp_object));
        prev->next = obj->next;

        if(HAS_LOCK(obj->info))
            remove_mutex(obj);
        std::free(obj);
    }
}

void sweep_memory(sharp_object *heap) {
    sharp_object *prev = nullptr;
    if(heap != nullptr) {
        guard_mutex(gc_lock)
        heap = heap->next;
    }

    while(heap != nullptr)
    {
        CHECK_STATE

        if(MARKED(heap->info) && GENERATION(heap->info) != gc_perm) {
            if(heap->refCount > 0) {
                MARK(heap->info, 0);
            } else {

                auto next = heap->next;
                sweep(prev, heap);
                heap = next;
                continue;
            }
        }

        prev = heap;
        heap = heap->next;
    }
}

void mark_and_sweep(sharp_object *heap) {
    mark_memory(heap);
    sweep_memory(heap);
}

/**
 *
 * This function decides which generation we need to point to based
 * On the current time slice we are inside of. Below is a general explication of this
 * process.
 *
 * [                           1 second                           ]
 * [         young gen.          ][     adult gen.   ][  old gen. ]
 *      ^ ~50% of the time we collect   |              ^ ~15% of the time we collect "old"
 *      the young generation pool as    |              or long-term use objects
 *      this is the most volatile.      |
 *                                      ^ ~35% of the time we collect
 *                                      any young objects that have graduated
 *                                      or "aged" to being an adult
 * @return Returns the gc heap generation
 */
sharp_object* select_heap_collection() {
    auto currentTime = NANO_TOMILL(Clock::realTimeInNSecs());

    if(COLLECTION_WINDOW_YOUNG(currentTime)) {
        return gc.yMemHead;
    } else if(COLLECTION_WINDOW_ADULT(currentTime)) {
        return gc.aMemHead;
    } else return gc.oMemHead;
}

bool collection_allowed() {
    return gc.state == RUNNING || gc.state == SLEEPING;
}

const Int MEMORY_POOL_SAMPLE_SIZE = 500;
uInt memoryPoolResults[MEMORY_POOL_SAMPLE_SIZE];
Int samplesReceived =0;

void update_threshold() {
    if (samplesReceived == MEMORY_POOL_SAMPLE_SIZE) {
        size_t total = 0, avg;
        for (long i = 0; i < MEMORY_POOL_SAMPLE_SIZE; i++) {
            total += memoryPoolResults[i];
        }

        avg = total / MEMORY_POOL_SAMPLE_SIZE;
        samplesReceived = 0;
        if((avg + (0.15 * avg)) < gc.memoryLimit)
            gc.memoryThreshold = avg + (0.15 * avg);
        else gc.memoryThreshold = avg;
    } else {
        memoryPoolResults[samplesReceived++] = gc.managedBytes;
    }
}

void gc_collect(collection_policy policy) {
    if(collection_allowed())
    {
        switch(policy) {

            case policy_partial_sweep: {
                mark_and_sweep(
                    select_heap_collection()
                );
                break;
            }

            case policy_full_sweep: {
                suspend_all_threads(true);
                mark_and_sweep(
                   gc.yMemHead
                );
                mark_and_sweep(
                   gc.aMemHead
                );
                mark_and_sweep(
                   gc.oMemHead
                );
                resume_all_threads(true);
                break;
            }
        }

        update_threshold();
    }
}

void send_message(collection_policy policy) {
    guard_mutex(gc_lock)
    gc.message_queue.push_back(policy);
}

void sedate_gc() {
    guard_mutex(gc_lock)
    auto gcThread = get_thread(gc_threadid);

    if(gc.state != SLEEPING && gcThread && gcThread->state == THREAD_RUNNING) {
        gc.state = SLEEPING;
        suspend_and_wait(gcThread, true);
    }
}

void wake_gc() {
    guard_mutex(gc_lock)
    auto gcThread = get_thread(gc_threadid);

    if(gc.state == SLEEPING && gcThread) {
        gc.state = RUNNING;
        suspend_and_wait(gcThread, true);
    }
}

void kill_gc() {
    auto gcThread = get_thread(gc_threadid);

    {
        guard_mutex(gc_lock)
        if(gcThread->state == THREAD_RUNNING) {
            send_interrupt_signal(gcThread);
        }
    }

    wait_for_thread_exit(gcThread);
}

void gc_main_loop()
{
    for(;;) {
        check_state:
        CHECK_STATE

        message:
        while (!gc.message_queue.empty()) {
            collection_policy policy;

            {
                guard_mutex(gc_lock)
                policy = *gc.message_queue.end();
                gc.message_queue.pop_back();
            }

            gc_collect(policy);
        }

        do {
            __usleep(GC_CLOCK_CYCLE);

            if(gc.state == SLEEPING) sleep_gc();
            if(!gc.message_queue.empty()) goto message;

            if(GC_COLLECT_MEM()) {
                /**
                 * Attempt to collect objects based on the appropriate
                 * conditions. This call does not guarantee that any collections
                 * will happen
                 */
                gc_collect(policy_partial_sweep);
            }

            if(thread_self->signal)
                goto check_state;
        }  while(true);
    }
}

#ifdef WIN32_
DWORD WINAPI
#endif
#ifdef POSIX_
void*
#endif
gc_main(void *pVoid) {
    thread_self = (sharp_thread*)pVoid;
    thread_self->state = THREAD_RUNNING;
    gc.state = RUNNING;
    set_thread_priority(thread_self, THREAD_PRIORITY_LOW);

    try {
        gc_main_loop();
    } catch(vm_exception &e){
        /* Should never happen */
        enable_exception_flag(thread_self, true);
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    shutdown_thread(thread_self);
    gc.state = SHUTDOWN;
#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void print_memory_diagnostics() {
    cout << "Memory DIagnostics\n";
    cout << "====================================\n\n";

    long types[type_label+1];
    long references[101];
    long young = 0, adult = 0, old = 0;
    cout << "young objects: ";

    auto obj = gc.yMemHead;
    while(obj != nullptr) {
        types[obj->type]++;
        young++;
        if(obj->refCount < 100) {
            if(obj->refCount >= 0)
                references[obj->refCount]++;
        } else {
            references[100]++;
        }
        obj = obj->next;
    }
    cout << young << endl;

    cout << "adult objects: ";
    obj = gc.aMemHead;
    while(obj != nullptr) {
        types[obj->type]++;
        adult++;
        if(obj->refCount < 100) {
            if(obj->refCount >= 0)
                references[obj->refCount]++;
        } else {
            references[100]++;
        }
        obj = obj->next;
    }
    cout << adult << endl;

    cout << "old objects: ";
    obj = gc.aMemHead;
    while(obj != nullptr) {
        types[obj->type]++;
        old++;
        if(obj->refCount < 100) {
            if(obj->refCount >= 0)
                references[obj->refCount]++;
        } else {
            references[100]++;
        }
        obj = obj->next;
    }
    cout << old << endl;

    int highestIndex = 0;
    int highestCount = 0;
    for(Int i =0; i < (type_label + 1); i++) {
        if(types[i] > highestCount) {
            highestCount = types[i];
            highestIndex = i;
        }
    }

    cout << "most found type: " << highestIndex << ":" << highestCount << endl;
    highestIndex = 0;
    highestCount = 0;
    for(Int i =0; i < (101); i++) {
        if(references[i] > highestCount) {
            highestCount = references[i];
            highestIndex = i;
        }
    }

    cout << "most common ref amount: " << highestIndex << ":" << highestCount << endl;
}

void gc_startup() {
    start_daemon_thread(
            gc_main,
            create_gc_thread()
    );
}


void sleep_gc() {
    sharp_thread* self = thread_self;
    gc.state = SLEEPING;
    self->state = THREAD_SUSPENDED;

    while(gc.state == SLEEPING) {
        __os_yield();
        __usleep(2);
        if(self->state != THREAD_RUNNING)
            break;
    }

    if(self->state == THREAD_SUSPENDED)
        self->state = THREAD_RUNNING;
    gc.state = RUNNING;
}


