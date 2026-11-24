//
// Created by bknun on 9/16/2022.
//
// What the garbage collector needs around it:
//
//   - the count of managed bytes and the memory limit     (reserve_bytes ...)
//   - the list of every object that exists                (push_object)
//   - the table of object locks                           (create_mutex ...)
//   - the garbage collector's helper thread               (gc_startup ...)
//   - the bits packed into sharp_object::info             (the macros below)
//
// The collector itself - deciding which objects are dead and freeing them - is
// in tracing_collector.h/.cpp. Read the top of tracing_collector.h first.
//
// HISTORY: objects used to be freed by reference counting, with a mark & sweep
// over three generations (young / adult / old) that looked for objects whose
// count had reached zero. All of that is gone; nothing counts references any
// more.
//

#ifndef SHARP_GARBAGE_COLLECTOR_H
#define SHARP_GARBAGE_COLLECTOR_H

#include "../../../stdimports.h"
#include "../../util/linked_list.h"
#include "sharp_object.h"
#include <atomic>
#include <vector>

struct fib_mutex;
struct fiber;

/**
 * Stored in two bits of sharp_object::info (GENERATION / SET_GENERATION).
 * See GENERATIONS in tracing_collector.h.
 */
enum collection_generation
{
    gc_normal = 0, // YOUNG: created since the last collection. Freed by the next collection
                   // of any kind when nothing refers to it any more.

    gc_old = 1,    // survived a collection. Only a full collection looks at it again.

    /**
     * A permanent object: it lives for the whole program and is never freed,
     * and everything it refers to stays alive with it (the collector treats it
     * as a root). The static object of every class and the reflection objects
     * built at startup are permanent. Set with tgc_make_permanent() only - the
     * collector keeps a list of them.
     */
    gc_perm  = 3
};

/** What OP_GC_STATE reports to the program (std library: gc state). */
enum gc_state
{
    RUNNING,    // collections happen when memory is needed
    SLEEPING,   // manual mode: only collects when the program asks; the memory limit is "out of memory"
    SHUTDOWN    // killed by the program or the VM is exiting: never collects
};

#define GC_CLOCK_CYCLE (500) // microseconds between two wake-ups of the helper thread

struct garbage_collector {
    gc_state state;
    std::atomic<uInt> managedBytes;         // bytes handed out through reserve_bytes
    uInt memoryLimit;           // the program's limit (-mem)
    std::atomic<sharp_object*> objects;     // YOUNG objects, newest first (linked through ->next): those made by
                                            // threads that do not keep a list of their own (sharp_thread::youngObjects)
    // The old and the permanent objects; only the collector changes it. An array
    // of pointers and not a list through ->next: a full collection can then ask
    // the CPU for the objects it will look at next (a prefetch) instead of
    // waiting for each one in turn - old objects are scattered all over memory.
    std::vector<sharp_object*> oldObjects;
    linkedlist<fib_mutex*> f_locks;
};

extern garbage_collector gc;

void reserve_bytes(size_t bytes, bool unsafe);
void release_bytes(size_t bytes);
void set_memory_limit(Int limit);
void push_object(sharp_object*);
void push_shared_object(sharp_object*);
struct sharp_thread;
void count_uncounted_bytes(sharp_thread *thread);   // a thread's own byte count handed to the shared counters
#define COUNT_BYTES_EVERY (64 * 1024)               // ... which reserve_bytes does every this many bytes
fib_mutex* create_mutex(sharp_object*);
void remove_mutex(sharp_object*);
void release_all_mutexes(fiber *task);
void print_memory_diagnostics();
void gc_startup();
void sedate_gc();
void wake_gc();
void kill_gc();

// object info macros
#define GENERATION_MASK 0x3
#define CLASS_MASK 0xfffffff
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

/*
 * THE DEFAULT CEILING OF THE HEAP (what -mem replaces)
 *
 * A quarter of the machine's memory, and never less than MINIMUM_HEAP_SIZE:
 * default_heap_size() in garbage_collector.cpp works it out when the VM starts.
 * It used to be a fixed 250 MB, which every real use had to override (the test
 * suite ran with -mem 1gb, the benchmarks with 2.8 GB, the compiler compiling
 * itself with 4 GB). The number is only a CEILING - the collector runs whenever
 * the heap has about doubled since its last run, whatever the ceiling is - so a
 * higher default costs a small program nothing.
 *
 *      C++ comparison: none (C++ has no heap limit); it is Java's rule for -Xmx:
 *      a quarter of physical memory.
 */
#define MINIMUM_HEAP_SIZE (MB_TO_BYTES(64))     // the owner's choice (2026-11-15): on a small machine, a quarter may be little
#define DEFAULT_HEAP_SIZE (default_heap_size())

uInt default_heap_size();

#define guard_mutex(mut) \
    std::lock_guard<recursive_mutex> guard((mut));

#endif //SHARP_GARBAGE_COLLECTOR_H
