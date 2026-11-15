//
// Created by bknun on 9/16/2022.
//
// Memory accounting, the list of all objects, the table of object locks and
// the garbage collector's helper thread. The collector itself is in
// tracing_collector.cpp (see garbage_collector.h for the split).
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
#include "../telemetry/telemetry.h"
#include "tracing_collector.h"

garbage_collector gc;
recursive_mutex gc_lock;    // guards gc.managedBytes, gc.objects and gc.f_locks

// ---------------------------------------------------------------------------------
// Memory accounting
// ---------------------------------------------------------------------------------

/**
 * Count `bytes` more of managed memory.
 *
 * unsafe = true:  just count (memory the runtime needs for itself, e.g. a
 *                 fiber's stack; cannot fail).
 * unsafe = false: memory for objects. This is where the collector learns how
 *                 much the program allocates; it may ask for a collection (which
 *                 then runs at the thread's next safe point, never here) and it
 *                 fails with "out of memory" when the limit is really reached.
 */
void reserve_bytes(size_t bytes, bool unsafe = false) {
    if(!unsafe && !tgc_note_allocation(bytes))
        throw vm_exception(vm.out_of_memory_except, "out of memory");

    guard_mutex(gc_lock)
    gc.managedBytes += bytes;
}

void set_memory_limit(Int limit) {
    gc.memoryLimit = limit;
}

/**
 * How much memory the machine has, in bytes; 0 when the operating system
 * will not say.
 */
static uInt physical_memory() {
#ifdef WIN32_
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if(GlobalMemoryStatusEx(&status))
        return (uInt) status.ullTotalPhys;
    return 0;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);
    if(pages > 0 && pageSize > 0)
        return (uInt) pages * (uInt) pageSize;
    return 0;
#endif
}

/**
 * The heap's ceiling when no -mem is given: a quarter of the machine's memory,
 * at least MINIMUM_HEAP_SIZE (see garbage_collector.h for the reasons).
 */
uInt default_heap_size() {
    uInt quarter = physical_memory() / 4;
    return quarter > MINIMUM_HEAP_SIZE ? quarter : MINIMUM_HEAP_SIZE;
}

void release_bytes(size_t bytes) {
    guard_mutex(gc_lock)

    // managedBytes is unsigned: `managedBytes - bytes >= 0` (what this used to
    // test) is always true, and releasing too much wrapped around to a huge number
    if(bytes <= gc.managedBytes) {
        gc.managedBytes -= bytes;
    } else {
        gc.managedBytes = 0;
    }
}

/** A new object exists: put it on the list the collector sweeps. */
void push_object(sharp_object *o) {
    {
        guard_mutex(gc_lock)
        o->next = gc.objects;
        gc.objects = o;
    }

    tgc_note_object();
}

// ---------------------------------------------------------------------------------
// Object locks ( lock(x) { } ): one fib_mutex per object that was ever locked
// ---------------------------------------------------------------------------------

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

/** The object is being freed: its lock goes with it. */
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
        mut->count = 0;
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
            item->data->count = 0;
        }
    });
}

// ---------------------------------------------------------------------------------
// Sleep / wake / kill (the program can ask for these: OP_GC_SLEEP ...)
// ---------------------------------------------------------------------------------

/** No collections until wake_gc(), except when the memory limit is reached. */
void sedate_gc() {
    guard_mutex(gc_lock)
    if(gc.state == RUNNING)
        gc.state = SLEEPING;
}

void wake_gc() {
    guard_mutex(gc_lock)
    if(gc.state == SLEEPING)
        gc.state = RUNNING;
}

/** Stop the helper thread; after this nothing is collected any more. */
void kill_gc() {
    auto gcThread = get_thread(gc_threadid);
    if(gcThread == nullptr) return;

    {
        guard_mutex(gc_lock)
        if(gcThread->state == THREAD_RUNNING) {
            send_interrupt_signal(gcThread);
        }
    }

    wait_for_thread_exit(gcThread);
}

// ---------------------------------------------------------------------------------
// The helper thread
// ---------------------------------------------------------------------------------
//
// Collections run on the threads that run Sharp code, at their safe points.
// This thread does not collect. It exists for one job: a request for a
// collection that no running thread has noticed (it was made by a thread that
// does not run Sharp code, or the signal bit was overwritten) is passed on to
// every running thread once per clock cycle - see tgc_nudge().

void gc_main_loop()
{
    for(;;) {
        CHECK_STATE

        __usleep(GC_CLOCK_CYCLE);
        tgc_nudge();
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
    TELEMETRY(telemetry_name_thread("garbage collector");)
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

void gc_startup() {
    start_daemon_thread(
            gc_main,
            create_gc_thread()
    );
}

/** Printed when the program runs out of memory. */
void print_memory_diagnostics() {
    cout << "Memory Diagnostics\n";
    cout << "====================================\n\n";

    long types[type_label+1] = {};
    long total = 0, permanent = 0;

    {
        guard_mutex(gc_lock)
        for(auto obj = gc.objects; obj != nullptr; obj = obj->next) {
            types[obj->type]++;
            total++;
            if(GENERATION(obj->info) == gc_perm) permanent++;
        }
    }

    cout << "objects: " << total << " (" << permanent << " permanent)" << endl;
    cout << "managed memory: " << (gc.managedBytes / 1024) << " kb of " << (gc.memoryLimit / 1024) << " kb" << endl;

    int highestIndex = 0;
    long highestCount = 0;
    for(Int i =0; i < (type_label + 1); i++) {
        if(types[i] > highestCount) {
            highestCount = types[i];
            highestIndex = i;
        }
    }

    cout << "most found type: " << highestIndex << ":" << highestCount << endl;
}
