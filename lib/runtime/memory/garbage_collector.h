//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_GARBAGE_COLLECTOR_H
#define SHARP_GARBAGE_COLLECTOR_H

#include "../../../stdimports.h"
#include "../../util/linked_list.h"
#include "sharp_object.h"
#include <atomic>

struct fib_mutex;
struct fiber;

enum collection_generation
{
    gc_young = 0, // collected ~50% of the time/sec
    gc_adult = 1, // collected ~35% of the time/sec
    gc_old   = 2, // collected ~15% of the time/sec

    /**
     * This is a permanent generation. objects promoted to this generation will live
     * for the entirety of the program and will not be garbage collected until the end of
     * the program. Objects in this state must be set manually
     */
    gc_perm  = 3
};

enum gc_state
{
    RUNNING,
    SLEEPING,
    SHUTDOWN
};

enum collection_policy
{
    policy_full_sweep,
    policy_partial_sweep
};

// time is defined in ms
#define COLLECTION_TIME (1000)
#define COLLECTION_WINDOW_YOUNG(tm) ((COLLECTION_TIME - (tm % COLLECTION_TIME)) > (COLLECTION_TIME / 50))
#define COLLECTION_WINDOW_ADULT(tm) ((COLLECTION_TIME - (tm % COLLECTION_TIME)) > (COLLECTION_TIME / 15))
#define GC_CLOCK_CYCLE (2000) // 2ms

struct garbage_collector {
    gc_state state;
    uInt managedBytes;
    uInt memoryLimit;
    uInt memoryThreshold;
    sharp_object *yMemHead;
    sharp_object *aMemHead;
    sharp_object *oMemHead;
    linkedlist<fib_mutex*> f_locks;
    std::list<collection_policy> message_queue;
};

extern garbage_collector gc;

void reserve_bytes(size_t bytes, bool unsafe);
void release_bytes(size_t bytes);
void set_memory_limit(Int limit);
void set_memory_threshold(Int threshold);
void push_object(sharp_object*);
void gc_collect(collection_policy);
void send_message(collection_policy policy);
fib_mutex* create_mutex(sharp_object*);
void release_all_mutexes(fiber *task);
void gc_startup();
void sleep_gc();
void sedate_gc();
void wake_gc();
void kill_gc();

#define GC_COLLECT_MEM() ( gc.managedBytes >= gc.memoryThreshold )

// object info macros
#define GENERATION_MASK 0x3
#define CLASS_MASK 0xffffff7
#define GENERATION(inf) ((inf >> 28) & GENERATION_MASK)
#define CLASS(inf) (inf & CLASS_MASK)
#define MARKED(inf) ((inf >> 30) & 1UL)
#define HAS_LOCK(inf) ((inf >> 31))
#define IS_CLASS(o) (o->type == type_class)
#define MARK(inf, enable) (inf ^= (-(unsigned long)enable ^ inf) & (1UL << 30))
#define SET_LOCK(inf, enable) (inf ^= (-(unsigned long)enable ^ inf) & (1UL << 31))
#define SET_GENERATION(inf, gen) (inf = (uint32_t)(CLASS(inf) | (gen << 28) | (MARKED(inf) << 30) | (HAS_LOCK(inf) << 31)))
#define SET_INFO(inf, k, gen) (inf = (uint32_t)(k |  (gen << 28)))

#define CHECK_STATE \
    if(hasSignal(thread_self->signal, tsig_suspend)) \
        suspend_self(); \
    if(thread_self->state == THREAD_KILLED || hasSignal(thread_self->signal, tsig_kill)) { \
        return; \
    }
/**
 * Bytes are used via the JEDEC Standard 100B.01
 */
#define KB_TO_BYTES(bytes) (((uInt)bytes)*1024)
#define MB_TO_BYTES(bytes) (((uInt)bytes)*1048576)
#define GB_TO_BYTES(bytes) (((uInt)(bytes))*1073741824)

#define DEFAULT_HEAP_SIZE (MB_TO_BYTES(250))
#define DEFAULT_HEAP_THRESHOLD (MB_TO_BYTES(128))

#define guard_mutex(mut) \
    std::lock_guard<recursive_mutex> guard((mut));

#endif //SHARP_GARBAGE_COLLECTOR_H
