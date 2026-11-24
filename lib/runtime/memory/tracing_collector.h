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
// GENERATIONS: MOST COLLECTIONS LOOK AT NEW OBJECTS ONLY
// -----------------------------------------------------
// Most objects die young, and what has survived one collection usually lives
// on. Marking and sweeping EVERYTHING each time therefore spends most of its
// work on objects that are not going anywhere: with a million live objects a
// collection took 160 to 210 ms, however little had been allocated.
//
//      YOUNG   created since the last collection      list: gc.objects
//      OLD     survived a collection                  array: gc.oldObjects
//
//   MINOR collection (the usual one, after about 8 MB of new objects):
//      mark    from the roots, through YOUNG objects only. An old object is
//              alive by definition and is not followed.
//      sweep   the young list only. What was reached becomes OLD: it is taken
//              off the young list and put on the old one. Nothing is copied
//              and no object ever changes its address.
//   FULL collection (when the old objects take three times the bytes the last
//      full one left, when the memory limit is near, or when the program asks):
//      mark and sweep everything, as every collection used to.
//
// The cost of a minor collection follows what was created since the last one,
// not everything that is alive.
//
// THE WRITE BARRIER. A minor collection does not follow old objects, so a
// young object that only an OLD object refers to would be missed. Hence every
// store of a reference into an object goes through a write barrier
// (memory/sharp_object.h): an old object that a young one is stored into is
// REMEMBERED, and the next minor collection looks at its slots as if it were
// a root. After a minor collection no young object is left (the survivors are
// old), so the remembered set starts empty again.
//
//      This is the "sticky mark bit" design (Demers et al., POPL 1990). Ruby
//      (since 2.1) and Lua 5.4 are generational in the same non-moving way.
//
//      C++ comparison: none - C++ has no collector. Java, C# and V8 MOVE the
//      survivors instead, which is faster still but changes every address an
//      object is known by; here the JIT, object locks and native code all
//      hold plain addresses.
//
// THE TWO NUMBERS (measured on x86-64 Linux, November 2026: the benchmarks, the
// compiler compiling a program, and a program whose objects die just after
// they became old - the worst case for this design):
//      young budget 8 MB       2 and 4 MB: more collections, no shorter in total.
//                              16 and 32 MB: each minor collection takes longer
//                              (up to 130 ms at 32 MB in the worst case) and
//                              nothing ran faster. A budget that grows when
//                              many objects survive was tried too: no gain.
//      full at 300%            a full collection when the old objects take three
//                              times what the last full one left, and at least
//                              16 MB more. 200% ran more full collections for
//                              the same peak memory; 400% saved little more.
//
// TUNING, without building again (SHARP_GC, see TESTING IT):
//      young=<mb>      new objects between two collections          (default 8)
//      full=<percent>  a full collection when the old objects reach this much
//                      of what the last full one left               (default 300)
//
// MANUAL MODE: THE PROGRAM DECIDES WHEN (std.system, gc)
// ------------------------------------------------------
// A program that cannot afford a pause at a moment it did not choose - a game
// in the middle of a frame - switches the collector to MANUAL (gc.state ==
// SLEEPING). Then no collection starts on its own: not after the young budget,
// not when the old objects have grown, not at the memory limit. The program
// collects where a pause does not matter (tgc_request(true): a full one,
// tgc_request_young(): a minor one). If it never does, the limit is reached
// and the allocation fails with "out of memory" - there is no rescue.
// tgc_fact() is what it reads to decide: how many collections ran, how long
// they took, how much was allocated since the last one.
//
//      C# comparison: GCSettings.LatencyMode and GC.TryStartNoGCRegion.
//      Unity: GarbageCollector.GCMode.Manual. Go: debug.SetGCPercent(-1).
//
// OUT OF MEMORY: A RESERVE, A HOOK, ONE EXCEPTION
// ----------------------------------------------
// The memory limit (-mem) is what ordinary code may use. Past it lies a
// RESERVE - a 32nd of the limit, 4 MB at most - that only the handling of an
// out of memory may use: a handler that cannot make one object cannot print
// a message or drop a cache.
//
//      0 --------------------- the limit ---- the limit + the reserve
//          ordinary use                reserve
//
// An allocation that would cross the limit (and, in normal mode, that a full
// collection could not make room for) OPENS the reserve. Then, in this order:
//
//   1. THE HOOK, when the program has one (gc.on_out_of_memory). The
//      allocation goes through, into the reserve. At its next safe point the
//      thread wakes the thread that runs the hook and waits for it. A hook
//      that collects memory back under the limit ends it there: the reserve
//      closes, the program goes on where it was, nothing was raised.
//   2. ONE EXCEPTION, when there is no hook or the hook did not help: the next
//      allocation over the limit fails with an out_of_memory_exception, which
//      a try/catch takes like any other. Its handler has the reserve to work
//      in.
//   3. THE END, when the reserve is used up too. No third chance: the VM
//      prints its memory report and stops, as it always did.
//
// So an out of memory cannot follow an out of memory without end: one hook
// run, one exception, then the reserve runs out.
//
// IT IS OVER when a collection leaves memory under the limit again - there is
// nothing to clear: the reserve is a line in the accounting, and what the
// handler made are ordinary objects. An allocation that was simply too large
// (a 2 GB array) takes nothing; the next out of memory is a new one.
//
// Not catchable: the operating system itself having no memory left (malloc
// gave null). That ends the program as before.
//
//      Java comparison: OutOfMemoryError, with the same pre-made exception
//      object (vm.memoryExcept). Java has no reserve, which is why catching
//      it there is called unreliable.
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
//                                             the owner of task->ptr and
//                                             task->ptrOwner
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
//                                        (under stress every fourth collection
//                                        is a full one, the others are minor:
//                                        a missing write barrier shows up as
//                                        quickly as a missing root)
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
bool tgc_stress_is_on();        // -gcstress: every new object must go through tgc_note_object (no inline allocation)

/** A thread struct was created / is about to be freed. */
void tgc_register_thread(sharp_thread *thread);
void tgc_unregister_thread(sharp_thread *thread);

/**
 * Ask for a collection. It runs at the next safe point of a Sharp thread.
 * Ignored in manual mode (sedate_gc) unless `force` is set - the program asks -
 * and always ignored once the collector has been shut down. A forced one is a
 * full collection.
 */
void tgc_request(bool force = false);

/** The program asks for a MINOR collection, in either mode. */
void tgc_request_young();

/**
 * A number about the collector, for the program (OP_GC_FACT):
 *      0   collections so far, minor and full
 *      1   full collections so far
 *      2   how long the last collection took, in microseconds
 *      3   the longest one
 *      4   all of them together
 *      5   bytes allocated since the last collection
 * -1 for a number that does not exist.
 */
Int tgc_fact(Int which);

/**
 * Make an object permanent: never freed, and a root (everything it refers to
 * stays alive too). For objects built once at startup that the runtime keeps
 * using: the static object of each class, reflection objects.
 */
void tgc_make_permanent(sharp_object *o);
bool tgc_pending();

/** Called by the garbage collector thread: make sure a pending request is noticed. */
void tgc_nudge();

/** What the collector says to an allocation (OUT OF MEMORY, above). */
enum allocation_answer {
    allocation_fits,        // go on
    allocation_raises,      // it fails with an out of memory exception the program can catch
    allocation_is_fatal     // it fails, and the reserve is used up: the program ends
};

/**
 * Called by reserve_bytes() for every allocation of managed memory. Decides
 * whether it is time to ask for a collection, and whether the allocation
 * fits. mayFail = false only counts: the answer is always allocation_fits.
 */
allocation_answer tgc_note_allocation(size_t bytes, bool mayFail = true);

/** An allocation was answered allocation_is_fatal: nobody waits for the program's hook any longer. */
void tgc_out_of_memory_is_fatal();

/**
 * The program's out of memory hook (OP_MEMORY_HOOK; std.system, gc.on_out_of_memory):
 *      0   there is no hook any more
 *      1   there is a hook
 *      2   wait up to `millis` for an out of memory; 1 = run the hook now
 *          (the thread that calls this is the one that runs the hook)
 *      3   the hook has run
 */
Int tgc_memory_hook(Int command, Int millis);

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
