//
// The tracing garbage collector. Read tracing_collector.h first: it explains
// what a tracing collector is, what the roots are and what safe points and
// safe regions are. This file is the implementation, in this order:
//
//      1. configuration (options, environment)
//      2. the thread registry and safe regions
//      3. asking for a collection (allocation accounting, limits)
//      4. stopping the world
//      5. marking (roots, then everything reachable)
//      6. sweeping (freeing what was not marked)
//      7. tgc_safepoint(): the function that ties 4-6 together
//

#include "tracing_collector.h"
#include "garbage_collector.h"
#include "../virtual_machine.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../types/sharp_function.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/fiber/fiber.h"
#include "../multitasking/scheduler/scheduler.h"
#include "../../core/thread_state.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// defined in garbage_collector.cpp
extern recursive_mutex gc_lock;
void remove_mutex(sharp_object *o);

bool gc_tracing = false;

// ---------------------------------------------------------------------------------
// 1. Configuration
// ---------------------------------------------------------------------------------

static Int stressEvery = 0;         // > 0: ask for a collection every N allocated objects
static bool logging = false;        // one line per collection on stderr

void tgc_enable() { gc_tracing = true; }
void tgc_set_logging(bool on) { logging = on; }

void tgc_set_stress(Int everyNAllocations) {
    stressEvery = everyNAllocations > 0 ? everyNAllocations : 1;
    gc_tracing = true;
}

/**
 * SHARP_GC is a comma separated list:  trace   stress=<n>   log
 * It exists so a whole test run can be switched over without touching the
 * command line of every test:   SHARP_GC=trace,stress=1 bash suite.sh
 */
void tgc_configure_from_environment() {
    const char *value = getenv("SHARP_GC");
    if(value == nullptr) return;

    string text(value), word;
    stringstream words(text);
    while(std::getline(words, word, ',')) {
        if(word == "trace") tgc_enable();
        else if(word == "log") tgc_set_logging(true);
        else if(word.rfind("stress", 0) == 0) {
            Int n = 1;
            if(word.size() > 7 && word[6] == '=') n = (Int)atoll(word.c_str() + 7);
            tgc_set_stress(n);
        }
    }
}

// ---------------------------------------------------------------------------------
// 2. The thread registry and safe regions
// ---------------------------------------------------------------------------------
//
// THE PROTOCOL (three rules, everything else follows from them):
//
//   a. thread->gcSafe == 1 means "this thread is not touching objects and will
//      not start to without taking world_mut first".
//   b. A thread may set gcSafe to 1 at any time WITHOUT a lock (it only
//      promises to do less). It may set it back to 0 ONLY while holding
//      world_mut.
//   c. The collecting thread holds world_mut for the whole collection.
//
// So once the collector has seen gcSafe == 1 for a thread, that thread cannot
// get back to touching objects until the collection is over: rule b sends it
// through world_mut, which rule c keeps locked.
//
// LOCK ORDER: world_mut is always taken first. Nothing may wait for world_mut
// while holding task_mutex, gc_lock, registry_mut or a thread's own mutex -
// which is why tgc_leave_safe() must not be called with a runtime lock held.

static std::mutex world_mut;        // held by the collector while the world is stopped
static std::mutex registry_mut;     // guards `threads`
static std::vector<sharp_thread*> threads;

void tgc_register_thread(sharp_thread *thread) {
    std::lock_guard<std::mutex> guard(registry_mut);
    thread->gcSafe = 1;             // a thread that has not started touches nothing
    threads.push_back(thread);
}

void tgc_unregister_thread(sharp_thread *thread) {
    std::lock_guard<std::mutex> guard(registry_mut);
    threads.erase(std::remove(threads.begin(), threads.end(), thread), threads.end());
}

void tgc_enter_safe(sharp_thread *thread) {
    if(thread) __atomic_store_n(&thread->gcSafe, 1, __ATOMIC_SEQ_CST);
}

void tgc_leave_safe(sharp_thread *thread) {
    if(thread == nullptr) return;

    // waits here for as long as a collection is running (rule c)
    std::lock_guard<std::mutex> world(world_mut);
    __atomic_store_n(&thread->gcSafe, 0, __ATOMIC_SEQ_CST);
}

gc_safe_region::gc_safe_region() {
    sharp_thread *self = thread_self;

    if(self != nullptr && __atomic_load_n(&self->gcSafe, __ATOMIC_SEQ_CST) == 0) {
        thread = self;
        tgc_enter_safe(self);
    } else {
        thread = nullptr;           // not a Sharp thread, or already inside a region
    }
}

gc_safe_region::~gc_safe_region() {
    if(thread != nullptr)
        tgc_leave_safe(thread);
}

static void set_gc_signal(sharp_thread *thread, bool on) {
    guard_mutex(thread->mut)
    sendSignal(thread->signal, tsig_gc, (on ? 1 : 0));
}

// ---------------------------------------------------------------------------------
// 3. Asking for a collection
// ---------------------------------------------------------------------------------
//
// WHEN: after `trigger` bytes have been allocated since the last collection.
// The trigger is the amount of memory that survived the last collection, but
// at least MIN_TRIGGER. So the heap is allowed to roughly double between
// collections: little live data -> small, frequent, cheap collections; a lot of
// live data -> rare collections. The total work stays proportional to what the
// program allocates.

static const uInt MIN_TRIGGER = MB_TO_BYTES(8);

static std::atomic<bool> requested(false);
static std::atomic<uInt> bytesSinceCollection(0);
static std::atomic<uInt> objectsSinceCollection(0);
static std::atomic<uInt> trigger(MIN_TRIGGER);
static std::atomic<bool> lastCollectionLeftHeapFull(false);
static uInt collections = 0;

bool tgc_pending() { return requested.load(std::memory_order_relaxed); }

void tgc_request() {
    requested.store(true);

    // make the calling thread look at the request at its next safe point
    sharp_thread *self = thread_self;
    if(self != nullptr && !hasSignal(self->signal, tsig_gc))
        set_gc_signal(self, true);
}

/**
 * The garbage collector thread calls this every clock cycle. A request made by
 * a thread that does not run Sharp code (or whose signal bit got lost) would
 * otherwise never be seen: tell every running thread to look.
 */
void tgc_nudge() {
    if(!requested.load()) return;

    std::lock_guard<std::mutex> guard(registry_mut);
    for(sharp_thread *thread : threads) {
        if(__atomic_load_n(&thread->gcSafe, __ATOMIC_SEQ_CST) == 0
           && !hasSignal(thread->signal, tsig_gc))
            set_gc_signal(thread, true);
    }
}

void tgc_note_object() {
    uInt count = ++objectsSinceCollection;
    if(stressEvery > 0 && (count % (uInt)stressEvery) == 0)
        tgc_request();
}

/**
 * THE MEMORY LIMIT. A collection cannot run inside an allocation, so reaching
 * the limit cannot be answered with "collect now and try again" like before.
 * Instead:
 *
 *   - over the limit for the first time: ask for a collection and let the
 *     allocation through. The thread collects at its next safe point, a few
 *     instructions later.
 *   - still over the limit although a collection just ran (it could not free
 *     enough): out of memory.
 *   - more than a quarter over the limit (somebody allocates a lot without
 *     reaching a safe point): out of memory.
 */
bool tgc_note_allocation(size_t bytes) {
    uInt since = (bytesSinceCollection += bytes);
    if(since >= trigger.load(std::memory_order_relaxed))
        tgc_request();

    uInt wanted = gc.managedBytes + bytes;
    if(wanted >= gc.memoryLimit) {
        tgc_request();

        if(lastCollectionLeftHeapFull.load() || wanted >= gc.memoryLimit + gc.memoryLimit / 4)
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------
// Pinned and permanent objects
// ---------------------------------------------------------------------------------

static std::mutex pin_mut;
static std::unordered_map<sharp_object*, Int> pins;   // object -> how many times it is pinned

void tgc_pin(sharp_object *o) {
    if(o == nullptr) return;
    std::lock_guard<std::mutex> guard(pin_mut);
    pins[o]++;
}

void tgc_unpin(sharp_object *o) {
    if(o == nullptr) return;
    std::lock_guard<std::mutex> guard(pin_mut);
    auto it = pins.find(o);
    if(it != pins.end() && --it->second <= 0)
        pins.erase(it);
}

// ---------------------------------------------------------------------------------
// 4. Stopping the world
// ---------------------------------------------------------------------------------

/**
 * Wait until every registered thread except `self` is in a safe state.
 * Returns false if the virtual machine started shutting down while we waited
 * (the collection is then abandoned: memory is about to be given back anyway).
 */
static bool stop_the_world(sharp_thread *self) {
    for(uInt round = 0; ; round++) {
        bool allSafe = true;

        {
            std::lock_guard<std::mutex> guard(registry_mut);
            for(sharp_thread *thread : threads) {
                if(thread == self) continue;
                if(__atomic_load_n(&thread->gcSafe, __ATOMIC_SEQ_CST) != 0) continue;

                allSafe = false;

                // Ask it to stop. Asked again every so often: other code
                // rewrites the signal word and could drop our bit.
                if(round % 64 == 0)
                    set_gc_signal(thread, true);
            }
        }

        if(allSafe) return true;
        if(vm.state >= VM_SHUTTING_DOWN) return false;

        // Threads normally reach a safe point within microseconds: give up the
        // CPU a few times first, then sleep in short steps.
        if(round < 200) std::this_thread::yield();
        else std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

// ---------------------------------------------------------------------------------
// 5. Marking
// ---------------------------------------------------------------------------------
//
// The mark bit lives in sharp_object::info (MARKED / MARK in
// garbage_collector.h). Marking uses an explicit list of objects whose fields
// still have to be looked at instead of recursion: a linked list with a million
// nodes would otherwise need a million nested C++ calls.

static std::vector<sharp_object*> pending;     // marked, fields not followed yet
static std::vector<object*> interiorPointers;  // task->ptr values that point into some object

static inline void mark(sharp_object *o) {
    if(o != nullptr && !MARKED(o->info)) {
        MARK(o->info, 1);

        // numbers have nothing to follow (type <= type_var: HEAD is an array
        // of long double), everything else is an array of object slots
        if(o->type > type_var && o->node != nullptr)
            pending.push_back(o);
    }
}

static void mark_everything_reachable() {
    while(!pending.empty()) {
        sharp_object *o = pending.back();
        pending.pop_back();

        object *slot = o->node;
        for(uInt i = 0; i < o->size; i++, slot++)
            mark(slot->o);
    }
}

/**
 * One fiber: its stack up to sp and everything else it holds on to. Also
 * clears the stack slots above sp (see "STACK SLOTS ABOVE sp" in the header).
 * Called with task_mutex held, so the fiber cannot be freed underneath us.
 */
static void mark_fiber(fiber *fib, void *) {
    mark(fib->exceptionObject.o);
    mark(fib->fiberObject.o);

    // the object it is waiting to lock and the objects it has locked: a lock
    // is found again through the object's address, so the object must stay
    if(fib->f_lock != nullptr) mark(fib->f_lock->o);
    for(Int i = 0; i < fib->heldCount; i++)
        mark(fib->held[i].mut->o);

    if(fib->stack == nullptr) return;

    stack_item *end = fib->stack + fib->stackSize;
    for(stack_item *item = fib->stack; item <= fib->sp && item < end; item++)
        mark(item->obj.o);

    // task->ptr is the "current object slot" register of the fiber: a pointer
    // to a SLOT, which can be
    //   - a slot of its own stack: fine if at or below sp (marked above); a
    //     slot above sp is a value that was just popped and is about to be
    //     used, so it is kept and not cleared
    //   - a static field: always a root
    //   - a field or element INSIDE some object: that object must stay alive
    //     or the slot itself is freed. Which object it is, is found later by
    //     one walk over the heap (mark_owners_of_interior_pointers).
    object *keep = nullptr;
    if(fib->ptr != nullptr) {
        object *stackFirst = &fib->stack->obj, *stackLast = &(end - 1)->obj;

        if(fib->ptr >= stackFirst && fib->ptr <= stackLast) {
            if(fib->ptr > &fib->sp->obj || fib->sp < fib->stack) {
                keep = fib->ptr;
                mark(keep->o);
            }
        } else if(!(fib->ptr >= vm.staticHeap && fib->ptr < vm.staticHeap + vm.mf.classes)) {
            interiorPointers.push_back(fib->ptr);
        }
    }

    for(stack_item *item = fib->sp + 1; item < end; item++) {
        if(&item->obj != keep)
            item->obj.o = nullptr;
    }
}

static void for_each_heap_list(void (*visit)(sharp_object **head)) {
    visit(&gc.yMemHead);
    visit(&gc.aMemHead);
    visit(&gc.oMemHead);
}

/**
 * Find the objects that contain the slots collected in interiorPointers and
 * mark them. A slot belongs to object o when it lies inside o's slot array:
 *
 *      o->node <= slot < o->node + o->size
 *
 * The pointers are sorted once so each object needs one binary search.
 */
static void mark_owners_in(sharp_object **head) {
    for(sharp_object *o = *head; o != nullptr; o = o->next) {
        if(o->type <= type_var || o->node == nullptr) continue;

        auto first = std::lower_bound(interiorPointers.begin(), interiorPointers.end(), o->node);
        if(first != interiorPointers.end() && *first < o->node + o->size)
            mark(o);
    }
}

static void mark_owners_of_interior_pointers() {
    if(interiorPointers.empty()) return;

    std::sort(interiorPointers.begin(), interiorPointers.end());
    for_each_heap_list(mark_owners_in);

    interiorPointers.clear();
}

/** Permanent objects (gc_perm) are never freed and count as roots. */
static void mark_permanent_in(sharp_object **head) {
    for(sharp_object *o = *head; o != nullptr; o = o->next) {
        if(GENERATION(o->info) == gc_perm)
            mark(o);
    }
}

static void mark_roots() {
    // static fields and reflection objects of every class
    for(Int i = 0; i < vm.mf.classes; i++) {
        mark(vm.staticHeap[i].o);

        sharp_class *sc = vm.classes + i;
        mark(sc->reflect);
        if(sc->fields != nullptr) {
            for(Int j = 0; j < sc->totalFieldCount; j++)
                mark(sc->fields[j].reflect);
        }
    }

    for(Int i = 0; i < vm.mf.methods; i++)
        mark(vm.methods[i].reflect);

    mark(vm.memoryExcept.o);

    // objects held by native libraries
    {
        std::lock_guard<std::mutex> guard(pin_mut);
        for(auto &pin : pins)
            mark(pin.first);
    }

    // threads
    {
        std::lock_guard<std::mutex> guard(registry_mut);
        for(sharp_thread *thread : threads) {
            mark(thread->currentThread.o);
            mark(thread->args.o);
        }
    }

    // fibers (running, waiting and not yet started ones alike)
    sched_visit_fibers(mark_fiber, nullptr);
}

// ---------------------------------------------------------------------------------
// 6. Sweeping
// ---------------------------------------------------------------------------------

static uInt sweptObjects, sweptBytes, liveObjects;

static void sweep_list(sharp_object **head) {
    sharp_object **link = head;     // the pointer that points at the object we look at

    while(*link != nullptr) {
        sharp_object *o = *link;

        if(MARKED(o->info)) {
            MARK(o->info, 0);       // ready for the next collection
            liveObjects++;
            link = &o->next;
            continue;
        }

        // not reached from any root: nothing can ever use it again
        *link = o->next;

        if(o->type <= type_var) {
            sweptBytes += sizeof(long double) * o->size;
            std::free(o->HEAD);
        } else if(o->node != nullptr) {
            sweptBytes += sizeof(object) * o->size;
            std::free(o->node);
        }

        if(HAS_LOCK(o->info))
            remove_mutex(o);

        sweptBytes += sizeof(sharp_object);
        sweptObjects++;
        std::free(o);
    }
}

// ---------------------------------------------------------------------------------
// 7. The safe point
// ---------------------------------------------------------------------------------

/** Runs with world_mut held and every other thread stopped. */
static void collect() {
    auto started = std::chrono::steady_clock::now();
    uInt bytesBefore = gc.managedBytes;

    {
        // task_mutex: no fiber is created or freed while we look at fibers.
        // gc_lock: nothing else changes the object lists or the lock table
        // (the scheduler's monitor thread frees dead fibers and their locks).
        // In this order: freeing a fiber takes task_mutex, then gc_lock.
        std::lock_guard<recursive_mutex> noFiberChanges(task_mutex);
        std::lock_guard<recursive_mutex> noHeapChanges(gc_lock);

        pending.clear();
        interiorPointers.clear();
        sweptObjects = sweptBytes = liveObjects = 0;

        for_each_heap_list(mark_permanent_in);
        mark_roots();
        mark_owners_of_interior_pointers();
        mark_everything_reachable();

        for_each_heap_list(sweep_list);
    }

    release_bytes(sweptBytes);
    collections++;

    // when to collect next (see section 3)
    uInt live = gc.managedBytes;
    uInt next = live > MIN_TRIGGER ? live : MIN_TRIGGER;
    if(live < gc.memoryLimit && next > gc.memoryLimit - live)
        next = gc.memoryLimit - live;       // do not plan to run past the limit
    if(next < MB_TO_BYTES(1)) next = MB_TO_BYTES(1);
    trigger.store(next);
    bytesSinceCollection.store(0);
    objectsSinceCollection.store(0);

    // "full" = the collection left less than 1/32 of the limit free
    lastCollectionLeftHeapFull.store(live + gc.memoryLimit / 32 >= gc.memoryLimit);

    if(logging) {
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - started).count();
        fprintf(stderr, "[gc %llu] freed %llu objects (%llu kb), %llu objects live, heap %llu kb -> %llu kb, pause %lld us\n",
                (unsigned long long)collections, (unsigned long long)sweptObjects,
                (unsigned long long)(sweptBytes / 1024), (unsigned long long)liveObjects,
                (unsigned long long)(bytesBefore / 1024), (unsigned long long)(live / 1024),
                (long long)micros);
    }
}

/**
 * The running thread saw tsig_gc at a safe point. Either another thread is
 * collecting (then we simply wait until it is done) or we do it ourselves.
 */
void tgc_safepoint() {
    sharp_thread *self = thread_self;
    if(self == nullptr) return;

    set_gc_signal(self, false);
    if(!gc_tracing) return;

    // Safe first, lock second (rule b): if another thread is collecting it is
    // waiting for exactly this flag while holding world_mut.
    tgc_enter_safe(self);
    std::lock_guard<std::mutex> world(world_mut);
    __atomic_store_n(&self->gcSafe, 0, __ATOMIC_SEQ_CST);

    // Somebody else may have collected while we waited for the lock.
    if(!requested.load()) return;

    if(stop_the_world(self))
        collect();

    requested.store(false);
}
