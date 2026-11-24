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
#include "../sni/sni_runtime.h"
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
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// defined in garbage_collector.cpp
extern recursive_mutex gc_lock;
void remove_mutex(sharp_object *o);

// ---------------------------------------------------------------------------------
// 1. Configuration
// ---------------------------------------------------------------------------------

static Int stressEvery = 0;         // > 0: ask for a collection every N allocated objects
static bool logging = false;        // one line per collection on stderr

// The two numbers that decide WHEN (tracing_collector.h, GENERATIONS). Bytes, not objects:
// a count would treat a 32 byte object and a 10 MB array as the same thing.
static uInt youngBudget = MB_TO_BYTES(8);   // new objects between two collections
static uInt fullPercent = 300;              // full collection: old bytes >= this much of what the last full one left
static const uInt FULL_MIN_GROWTH = MB_TO_BYTES(16);    // ... and at least this much more than it left

void tgc_set_logging(bool on) { logging = on; }
bool tgc_stress_is_on() { return stressEvery > 0; }

void tgc_set_stress(Int everyNAllocations) {
    stressEvery = everyNAllocations > 0 ? everyNAllocations : 1;
}

/**
 * SHARP_GC is a comma separated list:  stress=<n>   log   young=<mb>   full=<percent>
 * It exists so a whole test run can be switched over without touching the
 * command line of every test:   SHARP_GC=stress=1 bash suite.sh
 */
void tgc_configure_from_environment() {
    const char *value = getenv("SHARP_GC");
    if(value == nullptr) return;

    string text(value), word;
    stringstream words(text);
    while(std::getline(words, word, ',')) {
        if(word == "log") tgc_set_logging(true);
        else if(word.rfind("young=", 0) == 0) {
            Int mb = (Int)atoll(word.c_str() + 6);
            if(mb >= 1) youngBudget = MB_TO_BYTES(mb);
        }
        else if(word.rfind("full=", 0) == 0) {
            Int percent = (Int)atoll(word.c_str() + 5);
            if(percent > 100) fullPercent = (uInt)percent;
        }
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

    // what it kept to itself goes to the shared list and counters (garbage_collector.cpp,
    // ALLOCATING WITHOUT SHARED MEMORY): the thread is gone, its objects may not be
    count_uncounted_bytes(thread);
    sharp_object *o = thread->youngObjects;
    thread->youngObjects = nullptr;
    while(o != nullptr) {
        sharp_object *next = o->next;
        push_shared_object(o);
        o = next;
    }
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
// WHEN: after `trigger` bytes have been allocated since the last collection -
// the young budget, or less when the memory limit is near. Whether that
// collection is a minor or a full one is decided when it runs (collect_now).
// The total work stays proportional to what the program allocates.

static std::atomic<bool> requested(false);
static std::atomic<bool> fullRequested(false);     // the next collection must be a full one
static std::atomic<uInt> bytesSinceCollection(0);
static std::atomic<uInt> objectsSinceCollection(0);
static std::atomic<uInt> trigger(MB_TO_BYTES(8));
static std::atomic<bool> youngRequested(false);    // the program asked for a minor one (tgc_request_young)
static std::atomic<bool> lastCollectionLeftHeapFull(false);
static uInt collections = 0;

// What the program can ask about (tgc_fact). Written while the world is stopped, read by any thread.
static std::atomic<uInt> collectionsDone(0);
static std::atomic<uInt> fullCollectionsDone(0);
static std::atomic<uInt> lastPauseMicros(0);
static std::atomic<uInt> longestPauseMicros(0);
static std::atomic<uInt> totalPauseMicros(0);

bool tgc_pending() { return requested.load(std::memory_order_relaxed); }

/** make the calling thread look at the request at its next safe point */
static void notice_request_soon() {
    sharp_thread *self = thread_self;
    if(self != nullptr && !hasSignal(self->signal, tsig_gc))
        set_gc_signal(self, true);
}

void tgc_request(bool force) {
    if(gc.state == SHUTDOWN || (gc.state == SLEEPING && !force))
        return;

    requested.store(true);
    if(force) fullRequested.store(true);    // the program asked: look at everything
    notice_request_soon();
}

void tgc_request_young() {
    if(gc.state == SHUTDOWN)
        return;

    youngRequested.store(true);
    requested.store(true);
    notice_request_soon();
}

Int tgc_fact(Int which) {
    switch(which) {
        case 0: return (Int)collectionsDone.load();
        case 1: return (Int)fullCollectionsDone.load();
        case 2: return (Int)lastPauseMicros.load();
        case 3: return (Int)longestPauseMicros.load();
        case 4: return (Int)totalPauseMicros.load();
        case 5: {
            if(thread_self != nullptr) count_uncounted_bytes(thread_self);
            return (Int)bytesSinceCollection.load();
        }
        default: return -1;
    }
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
    if(stressEvery <= 0) return;        // the count is only used by -gcstress
    uInt count = ++objectsSinceCollection;
    if(stressEvery > 0 && (count % (uInt)stressEvery) == 0)
        tgc_request();
}

// ---- out of memory (tracing_collector.h, OUT OF MEMORY)
static std::atomic<bool> reserveOpen(false);    // an out of memory is being handled: allocations may use the reserve
static std::atomic<bool> raisedOnce(false);     // ... and its one exception has been raised
static std::atomic<bool> hookSet(false);        // the program has a hook for it (std.system, gc.on_out_of_memory)
static std::atomic<bool> hookPending(false);    // ... which has to run before anything is raised
static std::atomic<bool> hookRan(false);        // ... and has run for this out of memory
static std::atomic<sharp_thread*> hookThread(nullptr);  // the thread that runs it: it never waits for itself
static std::mutex hook_mut;
static std::condition_variable hookChanged;
static bool hookWanted = false;                 // guarded by hook_mut: a thread is waiting for the hook to run

/** the memory past the limit that only the handling of an out of memory may use */
static uInt memory_reserve() {
    uInt part = gc.memoryLimit / 32;
    return part < MB_TO_BYTES(4) ? part : MB_TO_BYTES(4);
}

/**
 * THE MEMORY LIMIT. A collection cannot run inside an allocation, so reaching
 * the limit cannot be answered with "collect now and try again" like before.
 * Instead, in NORMAL mode:
 *
 *   - over the limit for the first time: ask for a full collection and let the
 *     allocation through. The thread collects at its next safe point, a few
 *     instructions later.
 *   - still over the limit although a collection just ran (it could not free
 *     enough), or more than a quarter over it (somebody allocates a lot without
 *     reaching a safe point): out of memory.
 *
 * In MANUAL mode nobody collects but the program. It did not, and the limit is
 * reached: out of memory, at once.
 *
 * What "out of memory" is, is told in tracing_collector.h (OUT OF MEMORY): the
 * reserve opens, the program's hook runs, one exception is raised, and only
 * when the reserve is used up too the program ends.
 *
 * mayFail = false: only count (bytes that were allocated a while ago).
 */
allocation_answer tgc_note_allocation(size_t bytes, bool mayFail) {
    uInt since = (bytesSinceCollection += bytes);
    if(since >= trigger.load(std::memory_order_relaxed))
        tgc_request();
    if(!mayFail)
        return allocation_fits;

    uInt inUse = gc.managedBytes;
    uInt wanted = inUse + bytes;
    if(wanted < gc.memoryLimit)
        return allocation_fits;

    // memory came back under the limit since the last out of memory (an allocation that was simply
    // too large took nothing): the reserve is whole, and this is a new one
    if(reserveOpen.load() && inUse < gc.memoryLimit)
        reserveOpen.store(false);

    if(!reserveOpen.load()) {
        if(gc.state != SLEEPING && !lastCollectionLeftHeapFull.load() && wanted < gc.memoryLimit + gc.memoryLimit / 4) {
            tgc_request(true);
            return allocation_fits;
        }

        raisedOnce.store(false);
        hookRan.store(false);
        reserveOpen.store(true);
    }

    bool inReserve = wanted < gc.memoryLimit + memory_reserve();
    if(inReserve && hookSet.load() && !hookRan.load()) {
        // the hook first: this thread waits for it at its next safe point (tgc_safepoint)
        hookPending.store(true);
        notice_request_soon();
        return allocation_fits;
    }
    if(!raisedOnce.exchange(true))
        return allocation_raises;
    return inReserve ? allocation_fits : allocation_is_fatal;
}

/** the hook is not waited for any longer, by anybody */
static void release_hook_waiters() {
    std::lock_guard<std::mutex> guard(hook_mut);
    hookPending.store(false);
    hookWanted = false;
    hookChanged.notify_all();
}

void tgc_out_of_memory_is_fatal() {
    release_hook_waiters();
}

/**
 * The calling thread used the reserve and the program has a hook: wake the
 * thread that runs it and wait until it is done. A safe region: the hook
 * collects, and this thread counts as stopped for that.
 */
static void wait_for_memory_hook(sharp_thread *self) {
    gc_safe_region blocked;
    std::unique_lock<std::mutex> lock(hook_mut);
    if(!hookPending.load()) return;

    hookWanted = true;
    hookChanged.notify_all();
    while(hookPending.load() && hookSet.load() && vm.state == VM_RUNNING
          && self->state != THREAD_KILLED && !hasSignal(self->signal, tsig_kill))
        hookChanged.wait_for(lock, std::chrono::milliseconds(20));
}

Int tgc_memory_hook(Int command, Int millis) {
    switch(command) {
        case 0: {
            hookSet.store(false);
            release_hook_waiters();
            return 0;
        }
        case 1: {
            hookSet.store(true);
            return 0;
        }
        case 2: {
            hookThread.store(thread_self);      // who waits here is who runs the hook
            gc_safe_region blocked;
            std::unique_lock<std::mutex> lock(hook_mut);
            hookChanged.wait_for(lock, std::chrono::milliseconds(millis > 0 ? millis : 0), []{ return hookWanted; });
            if(!hookWanted) return 0;
            hookWanted = false;
            return 1;
        }
        case 3: {
            // still over the limit: the hook did not help, and the next allocation is told so
            hookRan.store(reserveOpen.load());
            release_hook_waiters();
            return 0;
        }
        default: return -1;
    }
}

// ---------------------------------------------------------------------------------
// Pinned and permanent objects
// ---------------------------------------------------------------------------------

static std::vector<sharp_object*> permanentObjects;    // guarded by pin_mut (declared below)

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

void tgc_make_permanent(sharp_object *o) {
    if(o == nullptr || GENERATION(o->info) == gc_perm) return;

    SET_GENERATION(o->info, gc_perm);
    {
        std::lock_guard<std::mutex> guard(pin_mut);
        permanentObjects.push_back(o);
    }

    // It counts as old from now on, so a minor collection does not follow it -
    // but what it refers to today may be young: have the next one look.
    gc_remember(o);
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
static bool minorCollection = false;           // this collection looks at young objects only

static inline void mark_now(sharp_object *o) {
    // an old (or permanent) object is alive by definition in a minor collection,
    // and what it refers to is found through the remembered set instead
    if(minorCollection && GENERATION(o->info) != gc_normal) return;

    if(!MARKED(o->info)) {
        MARK(o->info, 1);

        // numbers have nothing to follow (type <= type_var: HEAD is an array
        // of vm_cell), everything else is an array of object slots
        if(o->type > type_var && o->node != nullptr)
            pending.push_back(o);
    }
}

/*
 * WAITING FOR MEMORY, ONCE PER OBJECT. Marking reads the header of every object
 * it reaches, and the objects of a large heap are not in the processor's cache:
 * each read waited for memory, about 20 ns an object in a full collection.
 * So an object that is reached is not looked at straight away. Its memory is
 * asked for (a prefetch) and it waits in a short queue; by the time its turn
 * comes - MARK_AHEAD objects later - the header has arrived.
 *
 *      (Cher, Hosking, Vijaykumar: "Software prefetching for mark-sweep
 *       garbage collection", ASPLOS 2004. Java's and Go's collectors do the same.)
 */
#define MARK_AHEAD 16
static sharp_object *reached[MARK_AHEAD];   // reached, their headers on the way; nullptr = an empty place
static unsigned reachedAt = 0;

static inline void mark(sharp_object *o) {
    if(o == nullptr) return;

    // (a minor collection looks at objects made a moment ago: they are in the cache still,
    //  and the queue only cost time there - measured, 4% of a minor collection)
    if(minorCollection) { mark_now(o); return; }

    __builtin_prefetch(o);
    sharp_object *due = reached[reachedAt];
    reached[reachedAt] = o;
    reachedAt = (reachedAt + 1) % MARK_AHEAD;
    if(due != nullptr) mark_now(due);
}

/** look at every object still waiting in the queue; true when there was one */
static bool mark_the_reached() {
    bool any = false;
    for(unsigned i = 0; i < MARK_AHEAD; i++) {
        sharp_object *o = reached[i];
        if(o == nullptr) continue;
        reached[i] = nullptr;
        mark_now(o);
        any = true;
    }
    return any;
}

/** mark what the slots of `o` refer to */
static inline void mark_slots_of(sharp_object *o) {
    if(o->type <= type_var || o->node == nullptr) return;

    // A class instance has number fields in some of its slots: those are
    // not references and must not be followed (their bits are an integer
    // or a decimal, not an address).
    const bool *numbers = number_slots_of(o);
    object *slot = o->node;
    if(numbers == nullptr) {
        for(uInt i = 0; i < o->size; i++, slot++)
            mark(slot->o);
    } else {
        for(uInt i = 0; i < o->size; i++, slot++) {
            if(!numbers[i]) mark(slot->o);
        }
    }
}

static void mark_everything_reachable() {
    do {
        while(!pending.empty()) {
            sharp_object *o = pending.back();
            pending.pop_back();
            mark_slots_of(o);
        }
    } while(mark_the_reached() || !pending.empty());
}

// ---------------------------------------------------------------------------------
// The remembered set (the write barrier's side of a minor collection)
// ---------------------------------------------------------------------------------
//
// rememberedSet    OLD objects a young object was stored into since the last
//                  collection (write_barrier in memory/sharp_object.h): all of
//                  their slots are looked at
// rememberedRanges the same for LARGE ARRAYS, where only the elements that were
//                  stored into are looked at: (array, first element, how many).
//                  By index and not by address: an array that grows gets new
//                  memory for its elements (realloc_object).
// keptYoung        YOUNG objects that C++ code stored into a slot it could not
//                  name the owner of (copy_object): kept alive once, as roots
//
// Both are only added to between collections and emptied by every collection.
// A young object is never freed between collections and an old one only by a
// full collection (which empties the lists first), so no entry can dangle.

static std::mutex remembered_mut;
struct remembered_range { sharp_object *owner; uInt first; uInt count; };

static std::vector<sharp_object*> rememberedSet;
static std::vector<sharp_object*> keptYoung;

/*
 * The ranges of ONE thread. A program that fills a large array stores into it
 * millions of times, so this path takes no lock: every thread writes to its
 * own buffer, and the collector reads them all while the threads are stopped.
 * A thread that ends leaves what it had in `orphanRanges`.
 */
struct range_buffer {
    std::vector<remembered_range> ranges;
    range_buffer();
    ~range_buffer();
};

static std::vector<range_buffer*> rangeBuffers;         // guarded by remembered_mut
static std::vector<remembered_range> orphanRanges;      // guarded by remembered_mut

range_buffer::range_buffer() {
    std::lock_guard<std::mutex> guard(remembered_mut);
    rangeBuffers.push_back(this);
}

range_buffer::~range_buffer() {
    std::lock_guard<std::mutex> guard(remembered_mut);
    orphanRanges.insert(orphanRanges.end(), ranges.begin(), ranges.end());
    rangeBuffers.erase(std::remove(rangeBuffers.begin(), rangeBuffers.end(), this), rangeBuffers.end());
}

static thread_local range_buffer rangesOfThread;
static uInt rememberedSlots;        // slots a minor collection read because of the two lists above (for the log)

static inline bool remembered_by_range(const sharp_object *o) {
    return o->arrayFlag && o->type > type_var && o->size > LARGE_ARRAY_SLOTS;
}

void gc_remember_range(sharp_object *owner, Int first, Int count) {
    if(count <= 0 || first < 0) return;
    if(!remembered_by_range(owner)) { gc_remember(owner); return; }

    std::vector<remembered_range> &ranges = rangesOfThread.ranges;
    // the usual case is a list that is appended to: one range that grows
    if(!ranges.empty()) {
        remembered_range &last = ranges.back();
        if(last.owner == owner && (uInt)first >= last.first && (uInt)first <= last.first + last.count) {
            uInt end = (uInt)first + (uInt)count;
            if(end > last.first + last.count) last.count = end - last.first;
            return;
        }
    }
    ranges.push_back({ owner, (uInt)first, (uInt)count });
}

/** minor collection: the elements of large arrays that were stored into */
static void mark_ranges(std::vector<remembered_range> &ranges) {
    for(remembered_range &range : ranges) {
        sharp_object *o = range.owner;
        if(o->type <= type_var || o->node == nullptr) continue;
        uInt end = range.first + range.count;
        if(end > o->size) end = o->size;        // the array was made shorter since
        for(uInt i = range.first; i < end; i++)
            mark(o->node[i].o);
        if(end > range.first) rememberedSlots += end - range.first;
    }
    ranges.clear();
}

void gc_remember_store(sharp_object *owner, object *slot) {
    // Is `slot` one of owner's slots at all? task->ptrOwner is not cleared when
    // ptr moves on to a local or a static (multitasking/fiber/fiber.h), so the
    // store may have had nothing to do with this object. Then there is nothing
    // to remember - and the fiber forgets the owner, so that the next store
    // through the same ptr does not come here again.
    if(owner->type <= type_var || owner->node == nullptr
       || slot < owner->node || slot >= owner->node + owner->size) {
        sharp_thread *self = thread_self;
        if(self != nullptr && self->task != nullptr && self->task->ptrOwner == owner)
            self->task->ptrOwner = nullptr;
        return;
    }

    if(remembered_by_range(owner)) gc_remember_range(owner, (Int)(slot - owner->node), 1);
    else gc_remember(owner);
}

void gc_remember(sharp_object *owner) {
    std::lock_guard<std::mutex> guard(remembered_mut);
    if(owner->remembered) return;       // another thread was first
    owner->remembered = 1;
    rememberedSet.push_back(owner);
}

void gc_keep_young(object *slot, sharp_object *value) {
    // a slot of the running fiber's stack is a root already
    sharp_thread *self = thread_self;
    if(self != nullptr && self->task != nullptr && self->task->stack != nullptr) {
        fiber *task = self->task;
        if(slot >= &task->stack->obj && slot <= &(task->stack + (task->stackSize - 1))->obj)
            return;
    }

    std::lock_guard<std::mutex> guard(remembered_mut);
    if(keptYoung.empty() || keptYoung.back() != value)
        keptYoung.push_back(value);
}

/** minor collection: what the write barrier collected is looked at like a root */
static void mark_from_remembered_set() {
    std::lock_guard<std::mutex> guard(remembered_mut);
    rememberedSlots = 0;
    for(sharp_object *o : rememberedSet) {
        o->remembered = 0;
        mark_slots_of(o);
        if(o->type > type_var) rememberedSlots += o->size;
    }
    for(range_buffer *buffer : rangeBuffers)
        mark_ranges(buffer->ranges);
    mark_ranges(orphanRanges);
    for(sharp_object *o : keptYoung)
        mark(o);
    rememberedSet.clear();
    keptYoung.clear();
}

/** full collection: everything is followed anyway, the lists only have to be emptied */
static void forget_remembered_set() {
    std::lock_guard<std::mutex> guard(remembered_mut);
    for(sharp_object *o : rememberedSet)
        o->remembered = 0;
    rememberedSet.clear();
    for(range_buffer *buffer : rangeBuffers)
        buffer->ranges.clear();
    orphanRanges.clear();
    keptYoung.clear();
    rememberedSlots = 0;
}

/**
 * One fiber: its stack up to sp and everything else it holds on to. Also
 * clears the stack slots above sp (see "STACK SLOTS ABOVE sp" in the header).
 * Called with task_mutex held, so the fiber cannot be freed underneath us.
 */
static void mark_fiber(fiber *fib, void *) {
    mark(fib->exceptionObject.o);
    mark(fib->fiberObject.o);
    mark(fib->ptrOwner);        // the write barrier reads it (multitasking/fiber/fiber.h)

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

/**
 * Find the objects that contain the slots collected in interiorPointers and
 * mark them. A slot belongs to object o when it lies inside o's slot array:
 *
 *      o->node <= slot < o->node + o->size
 *
 * The pointers are sorted once so each object needs one binary search.
 *
 * A minor collection walks the young list only: an old object is not freed by
 * it, so a slot inside one stays valid whoever owns it.
 */
static inline void mark_if_owner_of_interior_pointer(sharp_object *o) {
    if(o->type <= type_var || o->node == nullptr) return;

    auto first = std::lower_bound(interiorPointers.begin(), interiorPointers.end(), o->node);
    if(first != interiorPointers.end() && *first < o->node + o->size)
        mark(o);
}

static void mark_owners_of_interior_pointers(bool oldObjectsToo) {
    if(interiorPointers.empty()) return;
    std::sort(interiorPointers.begin(), interiorPointers.end());

    for(sharp_object *o = gc.objects.load(); o != nullptr; o = o->next)
        mark_if_owner_of_interior_pointer(o);
    {
        // ... and the young objects every thread keeps on its own list
        std::lock_guard<std::mutex> guard(registry_mut);
        for(sharp_thread *thread : threads) {
            for(sharp_object *o = thread->youngObjects; o != nullptr; o = o->next)
                mark_if_owner_of_interior_pointer(o);
        }
    }

    if(oldObjectsToo) {
        for(sharp_object *o : gc.oldObjects)
            mark_if_owner_of_interior_pointer(o);
    }

    interiorPointers.clear();
}

static void mark_roots() {
    // the static fields of every class
    for(Int i = 0; i < vm.mf.classes; i++)
        mark(vm.staticHeap[i].o);

    mark(vm.memoryExcept.o);

    // permanent objects, and objects held by native libraries
    {
        std::lock_guard<std::mutex> guard(pin_mut);
        for(sharp_object *o : permanentObjects)
            mark(o);
        for(auto &pin : pins)
            mark(pin.first);
    }

    // threads
    {
        std::lock_guard<std::mutex> guard(registry_mut);
        for(sharp_thread *thread : threads) {
            mark(thread->currentThread.o);
            mark(thread->args.o);
            sni_mark_locals(thread, mark);      // objects a native function on this thread was given or made
        }
    }

    // fibers (running, waiting and not yet started ones alike)
    sched_visit_fibers(mark_fiber, nullptr);
}

// ---------------------------------------------------------------------------------
// 6. Sweeping
// ---------------------------------------------------------------------------------

static uInt sweptObjects, sweptBytes, liveObjects, promotedObjects;

/** an object nothing can reach any more: give its memory back */
static inline void free_dead_object(sharp_object *o) {
    if(o->type <= type_var) {
        sweptBytes += (size_t)element_size(o->type) * o->size;
        if(!o->inlineData) std::free(o->data);       // an inline payload goes with the header
    } else if(o->node != nullptr) {
        sweptBytes += sizeof(object) * o->size;
        if(!o->inlineData) std::free(o->node);
    }

    if(HAS_LOCK(o->info))
        remove_mutex(o);

    sweptBytes += sizeof(sharp_object);
    sweptObjects++;
    pool_free_swept(o);     // back to the object pool, or free() for a large one
}

/**
 * Full collection: the old objects. What was not marked is freed and the array
 * is closed up over the gaps. The objects are scattered all over memory, so the
 * loop asks for the ones it will reach in a moment (a prefetch) while it works
 * on the current one; without that it waits for memory once per object.
 */
static void sweep_old_objects() {
    std::vector<sharp_object*> &old = gc.oldObjects;
    size_t kept = 0, count = old.size();

    for(size_t i = 0; i < count; i++) {
        if(i + 12 < count) __builtin_prefetch(old[i + 12]);
        sharp_object *o = old[i];

        if(MARKED(o->info)) {
            MARK(o->info, 0);       // ready for the next collection
            old[kept++] = o;
            liveObjects++;
        } else {
            free_dead_object(o);    // not reached from any root: nothing can ever use it again
        }
    }

    old.resize(kept);
}

/**
 * Every collection: the young list. What was not marked is freed; what was
 * reached has survived a collection and becomes OLD - it leaves the young list
 * and its address is added to the old objects (the object stays where it is).
 *
 * There is one shared list and one per thread that runs Sharp code
 * (garbage_collector.cpp, ALLOCATING WITHOUT SHARED MEMORY). The shared one is
 * pushed to without a lock, so the sweep takes the whole of it for itself
 * first; anything pushed meanwhile stays young. The threads' own lists are
 * simply taken: their threads are stopped.
 */
static void sweep_young_objects(sharp_object *o);

static void sweep_young_list() {
    sweep_young_objects(gc.objects.exchange(nullptr));

    std::lock_guard<std::mutex> guard(registry_mut);
    for(sharp_thread *thread : threads) {
        sharp_object *own = thread->youngObjects;
        thread->youngObjects = nullptr;
        sweep_young_objects(own);
    }
}

static void sweep_young_objects(sharp_object *o) {
    while(o != nullptr) {
        sharp_object *next = o->next;
        if(next != nullptr) __builtin_prefetch(next);

        // (a permanent object is never marked by a minor collection and never freed)
        if(MARKED(o->info) || GENERATION(o->info) != gc_normal) {
            MARK(o->info, 0);
            if(GENERATION(o->info) == gc_normal)
                SET_GENERATION(o->info, gc_old);

            o->next = nullptr;
            gc.oldObjects.push_back(o);
            liveObjects++;
            promotedObjects++;
        } else {
            free_dead_object(o);
        }

        o = next;
    }
}

// ---------------------------------------------------------------------------------
// 7. The safe point
// ---------------------------------------------------------------------------------

static uInt liveAfterLastFull = 0;      // bytes the last full collection left
static uInt liveAfterLast = 0;          // bytes the last collection of any kind left: the old objects of today

/** one collection, minor or full. Runs with world_mut held and every other thread stopped. */
static void collect(bool full) {
    auto started = std::chrono::steady_clock::now();
    uInt bytesBefore = gc.managedBytes;
    minorCollection = !full;

    {
        // task_mutex: no fiber is created or freed while we look at fibers.
        // gc_lock: nothing else changes the lock table (the scheduler's monitor
        // thread frees dead fibers and their locks).
        // In this order: freeing a fiber takes task_mutex, then gc_lock.
        std::lock_guard<recursive_mutex> noFiberChanges(task_mutex);
        std::lock_guard<recursive_mutex> noHeapChanges(gc_lock);

        pending.clear();
        interiorPointers.clear();
        sweptObjects = sweptBytes = liveObjects = promotedObjects = 0;

        mark_roots();

        if(full) {
            forget_remembered_set();
        } else {
            mark_from_remembered_set();
        }

        mark_owners_of_interior_pointers(full);

        mark_everything_reachable();

        if(full) sweep_old_objects();
        sweep_young_list();
        pool_sweep_done();
    }

    release_bytes(sweptBytes);
    collections++;
    minorCollection = false;

    // what is alive now is all old: that is what the next decision looks at
    uInt live = gc.managedBytes;
    liveAfterLast = live;
    if(full) liveAfterLastFull = live;

    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count();
    collectionsDone++;
    if(full) fullCollectionsDone++;
    lastPauseMicros.store((uInt)micros);
    totalPauseMicros += (uInt)micros;
    if((uInt)micros > longestPauseMicros.load()) longestPauseMicros.store((uInt)micros);

    if(logging) {
        fprintf(stderr, "[gc %llu %s] freed %llu objects (%llu kb), %llu %s, heap %llu kb -> %llu kb, pause %lld us",
                (unsigned long long)collections, full ? "full" : "minor", (unsigned long long)sweptObjects,
                (unsigned long long)(sweptBytes / 1024),
                (unsigned long long)(full ? liveObjects : promotedObjects), full ? "objects live" : "objects became old",
                (unsigned long long)(bytesBefore / 1024), (unsigned long long)(live / 1024),
                (long long)micros);
        if(!full) fprintf(stderr, ", %llu remembered slots", (unsigned long long)rememberedSlots);
        fprintf(stderr, "\n");
    }
}

/** is it time to look at the old objects too? (tracing_collector.h, GENERATIONS) */
static bool full_collection_is_due() {
    if(fullRequested.exchange(false)) return true;

    // under -gcstress: three minor collections, then a full one
    if(stressEvery > 0 && collections % 4 == 3) return true;

    // the old objects have grown to fullPercent of what the last full collection left
    uInt due = liveAfterLastFull / 100 * fullPercent;
    if(due < liveAfterLastFull + FULL_MIN_GROWTH) due = liveAfterLastFull + FULL_MIN_GROWTH;
    if(liveAfterLast >= due) return true;

    // the memory limit is near: a minor collection cannot give back what old garbage holds
    return gc.managedBytes + youngBudget >= gc.memoryLimit;
}

/** Runs with world_mut held and every other thread stopped. */
static void collect_now() {
    // the program asked for a minor collection and nobody asked for a full one: a minor one it is,
    // whatever the old objects have grown to
    bool youngOnly = youngRequested.exchange(false) && !fullRequested.load();
    bool full = !youngOnly && full_collection_is_due();
    collect(full);

    // "full" heap = the collection left less than 1/32 of the limit free
    uInt live = gc.managedBytes;
    if(!full && !youngOnly && live + gc.memoryLimit / 32 >= gc.memoryLimit) {
        collect(true);      // the minor one was not enough: look at everything before giving up
        live = gc.managedBytes;
    }
    // (a minor one the program asked for says nothing about what a full one could free)
    bool heapFull = live + gc.memoryLimit / 32 >= gc.memoryLimit;
    if(!youngOnly || !heapFull) lastCollectionLeftHeapFull.store(heapFull);

    // an out of memory is over when memory is back under the limit: the reserve is whole again
    if(reserveOpen.load() && live < gc.memoryLimit) reserveOpen.store(false);

    // when to collect next (see section 3)
    uInt next = youngBudget;
    if(live < gc.memoryLimit && next > gc.memoryLimit - live)
        next = gc.memoryLimit - live;       // do not plan to run past the limit
    if(next < MB_TO_BYTES(1)) next = MB_TO_BYTES(1);
    trigger.store(next);
    bytesSinceCollection.store(0);
    objectsSinceCollection.store(0);
}

/**
 * The running thread saw tsig_gc at a safe point. Either another thread is
 * collecting (then we simply wait until it is done) or we do it ourselves.
 */
void tgc_safepoint() {
    sharp_thread *self = thread_self;
    if(self == nullptr) return;

    set_gc_signal(self, false);
    count_uncounted_bytes(self);    // what this thread allocated is in the counters the collection reads

    // out of memory, and the program has a hook for it: the hook runs before this thread goes on
    if(hookPending.load() && self != hookThread.load())
        wait_for_memory_hook(self);

    // Safe first, lock second (rule b): if another thread is collecting it is
    // waiting for exactly this flag while holding world_mut.
    tgc_enter_safe(self);
    std::lock_guard<std::mutex> world(world_mut);
    __atomic_store_n(&self->gcSafe, 0, __ATOMIC_SEQ_CST);

    // Somebody else may have collected while we waited for the lock.
    if(!requested.load()) return;

    if(stop_the_world(self))
        collect_now();

    requested.store(false);
}
