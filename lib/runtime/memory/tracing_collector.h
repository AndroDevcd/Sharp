//
// The tracing garbage collector.
//
// WHAT THIS REPLACES
// ------------------
// The runtime used to decide when an object dies by counting the references to
// it (sharp_object::refCount, changed by every copy_object). That needs every
// single place that stores or drops a reference to count correctly, forever:
// one missed increment frees an object that is still in use (a crash), one
// missed decrement keeps an object alive for the rest of the program (memory
// pressure). It also made every reference assignment take a global lock.
//
// A tracing collector does not count anything. When memory is needed it asks a
// different question: "starting from the places the program can still reach
// (the ROOTS), which objects can be reached?" Everything else is garbage.
//
//      C++ comparison: reference counting is std::shared_ptr everywhere.
//      Tracing is what Java, C#, Go and JavaScript engines do.
//
// HOW A COLLECTION WORKS (stop the world, mark, sweep)
// ----------------------------------------------------
//   1. STOP    every thread that runs Sharp code is brought to a SAFE POINT:
//              a place between two instructions where everything it refers to
//              is in a location the collector knows about (see below).
//   2. MARK    start at the roots and follow every reference, setting the mark
//              bit of each object that is reached.
//   3. SWEEP   walk the list of all objects: unmarked ones are freed, marked
//              ones get their mark bit cleared for the next collection.
//   4. RESUME  the threads continue.
//
// THE ROOTS (tracing_collector.cpp, mark_roots)
// ---------------------------------------------
//   - static fields of every class            vm.staticHeap
//   - reflection objects                      class / field / function ->reflect
//   - the out of memory exception             vm.memoryExcept
//   - every thread                            currentThread, args
//   - every fiber                             its stack from the bottom up to
//                                             sp, exceptionObject, fiberObject,
//                                             the object it waits to lock and
//                                             the objects it holds locks on,
//                                             and the owner of task->ptr
//   - objects native libraries hold           the pin table (tgc_pin)
//
// STACK SLOTS ABOVE sp
// --------------------
// A function that returned leaves its values behind in the slots above sp.
// They are not roots (the function is gone), and the collector sets them to
// null while the world is stopped. So a slot never points at a freed object:
// code that wrongly reads a leftover slot sees either a live object or null,
// never freed memory.
//
// SAFE POINTS AND SAFE REGIONS
// ----------------------------
// SAFE POINT: the interpreter's `state_check:` and the same check in JIT
// compiled code. They already look at thread->signal on every backward branch,
// call and return; the collector adds one signal bit, tsig_gc.
//
// SAFE REGION: a thread that is blocked (sleeping, waiting for a fiber, joining
// a thread, reading the keyboard, running a shell command) cannot reach a safe
// point, but it also cannot touch any object. Such code is wrapped in a
// `gc_safe_region`: the collector treats the thread as stopped, and the thread
// waits at the end of the region if a collection is still going on.
//
// A collection NEVER happens inside an allocation. Allocating only asks for a
// collection (tgc_request); it runs when the thread reaches its next safe
// point. That is what makes C++ code like
//
//      auto o = create_object(...);       // o is only in a C++ local here
//      copy_object(slot, create_object(...));
//
// safe without telling the collector about C++ locals.
//
// TESTING IT
// ----------
//      sharp -gcstress <n> program       collect after every n allocations
//                                        (a missing root shows up almost
//                                        immediately; best with a runtime built
//                                        with -fsanitize=address)
//      sharp -gclog program              one line per collection on stderr
//      SHARP_GC=stress=1,log             the same through the environment
//
// ADDING A NEW PLACE THAT HOLDS AN OBJECT
// ---------------------------------------
// If runtime code keeps a sharp_object* anywhere that outlives one instruction
// (a new field of fiber, sharp_thread, the vm, a table ...), add it to
// mark_roots() in tracing_collector.cpp. That is the whole rule, and
// -gcstress with the sanitizer build is how to check it.
//

#ifndef SHARP_TRACING_COLLECTOR_H
#define SHARP_TRACING_COLLECTOR_H

#include "../../../stdimports.h"
#include "sharp_object.h"

struct sharp_thread;

/** Read SHARP_GC from the environment. Call once at startup. */
void tgc_configure_from_environment();
void tgc_set_stress(Int everyNAllocations);
void tgc_set_logging(bool on);

/** A thread struct was created / is about to be freed. */
void tgc_register_thread(sharp_thread *thread);
void tgc_unregister_thread(sharp_thread *thread);

/**
 * Ask for a collection. It runs at the next safe point of a Sharp thread.
 * Ignored while the collector is asleep (sedate_gc) unless `force` is set, and
 * always ignored once it has been shut down.
 */
void tgc_request(bool force = false);

/**
 * Make an object permanent: never freed, and a root (everything it refers to
 * stays alive too). For objects built once at startup that the runtime keeps
 * using: the static object of each class, reflection objects.
 */
void tgc_make_permanent(sharp_object *o);
bool tgc_pending();

/** Called by the garbage collector thread: make sure a pending request is noticed. */
void tgc_nudge();

/**
 * Called by reserve_bytes() for every allocation of managed memory. Decides
 * whether it is time to ask for a collection. Returns false if the allocation
 * must fail with "out of memory".
 */
bool tgc_note_allocation(size_t bytes);

/** Called by push_object(): one more object exists. */
void tgc_note_object();

/** The safe point: called when the running thread sees tsig_gc. */
void tgc_safepoint();

/** Enter / leave a safe region by hand (prefer gc_safe_region). */
void tgc_enter_safe(sharp_thread *thread);
void tgc_leave_safe(sharp_thread *thread);

/**
 * Objects held by native code (SNI: a kept reference, sni/sni.h) are not in any place the
 * collector looks at, so they are listed here while they are held.
 */
void tgc_pin(sharp_object *o);
void tgc_unpin(sharp_object *o);

/**
 * Everything between the construction and destruction of one of these is a
 * safe region for the calling thread. Nesting is fine: only the outermost one
 * changes the thread's state.
 *
 *      {
 *          gc_safe_region blocked;
 *          std::this_thread::sleep_for(...);
 *      }   // waits here while a collection is running
 *
 * RULES for code inside a region: do not read or write objects, and do not
 * hold a runtime lock (task_mutex, thread_mutex, ...) when the region ends.
 */
struct gc_safe_region {
    gc_safe_region();
    ~gc_safe_region();
    sharp_thread *thread;   // null = nothing to undo (not a Sharp thread, or already in a region)
};

#endif //SHARP_TRACING_COLLECTOR_H
