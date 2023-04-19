//
// Created by bknun on 9/16/2022.
//

#include "garbage_collector.h"
#include "../virtual_machine.h"
#include "../../util/time.h"
#include "../multitasking/thread/thread_controller.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../../core/thread_state.h"

garbage_collector gc;
recursive_mutex gc_lock;

void reserve_bytes(size_t bytes) {
    guard_mutex(gc_lock)

    if((bytes + gc.managedBytes) >= gc.memoryLimit) {
        gc_collect();
    }

    if((bytes + gc.managedBytes) < gc.memoryLimit) {
        gc.managedBytes += bytes;
    } else {
        throw vm_exception(vm.out_of_memory_exception, "out of memory");
    }
}

void age_mem(sharp_object *prev, sharp_object *ager, sharp_object *newHeap)
{
    if(prev != nullptr && ager != nullptr) {
        prev->next = ager->next;
        if(newHeap->next == nullptr) {
            newHeap->next = ager;
            ager->next = nullptr;
        } else {
            auto tmp = newHeap->next;
            newHeap->next = ager;
            ager->next = tmp;
        }
    }
}

void mark_memory(sharp_object *heap) {
    sharp_object *pev = nullptr;

    while(heap != nullptr)
    {
        // todo: check thread state

        if(GENERATION(heap->info) <= gc_old) {
            if(heap->refCount == 0) {
                MARK(heap->info, 1);
            } else {

                auto next = heap->next;
                switch (GENERATION(heap->info)) {

                    case gc_young: {
                        age_mem(prev, heap, gc.aMemHead);
                        break;
                    }

                    case gc_adult: {
                        age_mem(prev, heap, gc.oMemHead);
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
            gc.managedBytes -= sizeof(double) * obj->size;
            std::free(obj->HEAD);
        } else if(obj->node != nullptr) {
            for(Int i = 0; i < obj->size; i++) {
                auto child = obj->node[i].o;

                if(child != nullptr) {
                    dec_ref(child)

                    if(child->refCount == 0) MARK(child->info, 1);
                }
            }

            gc.managedBytes -= sizeof(object) * obj->size;
            std::free(obj->node);
        }

        managedBytes -= sizeof(sharp_object);
        prev->next = obj->next;

        // todo: possibly need lock dropping here
        std::free(obj);
    }
}

void sweep_memory(sharp_object *heap) {
    sharp_object *prev = nullptr;

    while(heap != nullptr)
    {
        // todo: check thread state

        if(MARKED(heap->info)) {
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
    auto currentTime = NANO_TOMILLS(Clock::realTimeInNSecs());

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
        if((avg + (0.15 * avg)) < memoryLimit)
            memoryThreshold = avg + (0.15 * avg);
        else memoryThreshold = avg;
    } else {
        memoryPoolResults[samplesReceived++] = managedBytes;
    }
}

void gc_collect(collection_policy policy) {
    guard_mutex(gc_lock) // todo: we might be able to omit the mutex lock (protect update_threshold) and start the heap search on the first item only for partial sweep

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
                // todo: stop the world
                mark_and_sweep(
                   gc.yMemHead
                );
                mark_and_sweep(
                   gc.aMemHead
                );
                mark_and_sweep(
                   gc.oMemHead
                );
                break;
            }
        }

        update_threshold();
    }
}

void gc_main_loop()
{
    for(;;) {
        check_state:
        if(hasSignal(tself->signal, tsig_suspend))
            suspend_self();
        if(tself->state == THREAD_KILLED || hasSignal(tself->signal, tsig_kill)) {
            return;
        }

        message:
        while (!gc.message_queue.empty()) {
            guard_mutex(gc_lock)
            collection_policy policy;
            policy = *gc.message_queue.end();
            gc.message_queue.pop_back()

            gc_collect(policy);
        }

        do {
            __usleep(GC_CLOCK_CYCLE);

            if(state == SLEEPING) sedate_self();
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
    } catch(Exception &e){
        /* Should never happen */
        sendSignal(thread_self->signal, tsig_except, 1);
    }

    /*
     * Check for uncaught exception in thread before exit
     */
    shutdown_thread(thread_self);
#ifdef WIN32_
    return 0;
#endif
#ifdef POSIX_
    return nullptr;
#endif
}

void gc_startup() {
    start_daemon_thread(
            gc_main,
            create_gc_thread()
    );
}


void sedate_self() {
    Thread* self = thread_self;
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


