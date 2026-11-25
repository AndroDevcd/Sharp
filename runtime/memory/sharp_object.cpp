//
// Created by bknun on 9/16/2022.
//

#include "sharp_object.h"
#include "garbage_collector.h"
#include "../types/sharp_class.h"
#include "../error/vm_exception.h"
#include "memory_helpers.h"
#include "../types/sharp_field.h"
#include "../types/sharp_type.h"
#include "../multitasking/fiber/fiber.h"
#include "../multitasking/thread/sharp_thread.h"
#include "../multitasking/thread/thread_controller.h"
#include "../multitasking/thread_state.h"
#include "../types/access_flag.h"
#include "../telemetry/telemetry.h"
#include "../multitasking/scheduler/scheduler.h"
#include "../reflect/reflect_helpers.h"
#include "../virtual_machine.h"
#include "tracing_collector.h"
#include <atomic>
#include <mutex>

std::recursive_mutex lock_mut;

void init_struct(object *o) {
    o->o = nullptr;
}

void init_struct(sharp_object *o) {
    o->next = nullptr;
    o->type = 0;
    o->size = 0;
    o->info = 0;
    o->arrayFlag = 0;
    o->staticObject = 0;
    o->inlineData = 0;
    o->poolClass = 0;
    o->remembered = 0;
    o->data = nullptr;
}

static_assert(sizeof(sharp_object) % 8 == 0, "what is stored right behind the header holds 8-byte numbers");

/*
 * ONE BLOCK PER OBJECT. The header and its payload (the slots of
 * an instance, the elements of an array) used to be two allocations: malloc
 * for the header, calloc for the payload - and two frees when it died. Up to
 * INLINE_LIMIT bytes the payload now sits right behind the header in the same
 * block. Larger arrays keep their own block (calloc gets zeroed pages from the
 * system without writing to them, and realloc can grow them in place).
 *
 *      C++ comparison: std::make_shared (one block for the control block and
 *      the object) instead of std::shared_ptr<T>(new T).
 */
#define INLINE_LIMIT 1024

/*
 * OBJECT POOL: small blocks without malloc.
 *
 * Most objects are small and of a few sizes. The general purpose malloc/free
 * of the C library cost 150 to 250 machine instructions for each of them here
 * - above all because the collector frees a million blocks in one go, which
 * defeats malloc's own per-thread cache. The pool hands out blocks of fixed
 * sizes from free lists instead:
 *
 *      size class c (1..POOL_CLASSES)      a block of 16 + 16 * c bytes
 *                                          (32, 48, ... 288: the header plus up to 256 bytes)
 *
 *      allocating      take the first block of this thread's list for the class
 *                      (no lock); when it is empty, take POOL_BATCH blocks from
 *                      the shared list (one lock), or cut a new chunk into blocks
 *      freeing         only the collector frees. The sweep strings the dead
 *                      blocks of each class together and gives them to the
 *                      shared list in one go when it is done.
 *
 * Anything larger comes from malloc as before (poolClass = 0).
 *
 *      C++ comparison: a std::pmr::unsynchronized_pool_resource per thread in
 *      front of a shared one.
 *
 * A thread that ends gives the blocks left in its lists back to the shared
 * list (pool_thread_lists, below). Chunks are never returned to the system:
 * a program's small objects are reused for its whole run.
 */
#define POOL_BATCH 256
#define POOL_CHUNK_BYTES (256 * 1024)

struct pool_block { pool_block *next; };

static std::mutex pool_mut;
static pool_block *poolShared[POOL_CLASSES + 1];             // guarded by pool_mut

static_assert(SHARP_POOL_LISTS == POOL_CLASSES + 1, "sharp_thread::pool has one list per size class");

/** a thread ends: the free blocks it has left go to the shared lists */
void pool_give_back(void **lists) {
    std::lock_guard<std::mutex> guard(pool_mut);
    for(int c = 1; c <= POOL_CLASSES; c++) {
        pool_block *first = (pool_block*) lists[c];
        if(first == nullptr) continue;
        pool_block *last = first;
        while(last->next != nullptr) last = last->next;
        last->next = poolShared[c];
        poolShared[c] = first;
        lists[c] = nullptr;
    }
}

/**
 * The free blocks of the calling thread, by size class. A thread that runs
 * Sharp code keeps them in its sharp_thread (compiled code takes blocks from
 * there too: jit/x64/opcode/allocation.cpp); any other thread in a C++
 * thread_local, handed back when the thread ends.
 */
struct pool_thread_lists {
    void *list[POOL_CLASSES + 1] = {};
    ~pool_thread_lists() { pool_give_back(list); }
};
static thread_local pool_thread_lists poolOfThread;

static inline pool_block** pool_lists() {
    sharp_thread *self = thread_self;
    return (pool_block**)(self != nullptr ? self->pool : poolOfThread.list);
}
#define poolLocal (pool_lists())

static inline size_t pool_block_bytes(int c) { return POOL_BLOCK_BYTES(c); }

static void pool_refill(int c) {
    std::lock_guard<std::mutex> guard(pool_mut);
    pool_block *first = poolShared[c];

    if(first != nullptr) {
        // take up to POOL_BATCH blocks off the front of the shared list
        pool_block *last = first;
        for(int i = 1; i < POOL_BATCH && last->next != nullptr; i++) last = last->next;
        poolShared[c] = last->next;
        last->next = nullptr;
        poolLocal[c] = first;
        return;
    }

    // nothing free: cut a new chunk into blocks, all for this thread
    size_t bytes = pool_block_bytes(c), count = POOL_CHUNK_BYTES / bytes;
    char *chunk = (char*) malloc(count * bytes);
    if(chunk == nullptr) throw vm_exception(vm.out_of_memory_except, "out of memory");
    for(size_t i = 0; i < count; i++) {
        pool_block *block = (pool_block*)(chunk + i * bytes);
        block->next = i + 1 < count ? (pool_block*)(chunk + (i + 1) * bytes) : nullptr;
    }
    poolLocal[c] = (pool_block*) chunk;
}

static inline void* pool_alloc(int c) {
    pool_block *block = poolLocal[c];
    if(block == nullptr) {
        pool_refill(c);
        block = poolLocal[c];
    }
    poolLocal[c] = block->next;
    return block;
}

// the sweep: dead blocks of each class, strung together until the sweep is over
static pool_block *sweptFirst[POOL_CLASSES + 1], *sweptLast[POOL_CLASSES + 1];

void pool_free_swept(sharp_object *o) {
    int c = o->poolClass;
    if(c == 0) { std::free(o); return; }

    pool_block *block = (pool_block*) o;
    block->next = sweptFirst[c];
    if(sweptFirst[c] == nullptr) sweptLast[c] = block;
    sweptFirst[c] = block;
}

void pool_sweep_done() {
    std::lock_guard<std::mutex> guard(pool_mut);
    for(int c = 1; c <= POOL_CLASSES; c++) {
        if(sweptFirst[c] == nullptr) continue;
        sweptLast[c]->next = poolShared[c];
        poolShared[c] = sweptFirst[c];
        sweptFirst[c] = sweptLast[c] = nullptr;
    }
}

static sharp_object* new_object_with_payload(size_t bytes, bool unsafe) {
    if(bytes > INLINE_LIMIT) {
        auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);
        init_struct(o);
        o->data = calloc_mem<char>(bytes, 1, unsafe);
        return o;
    }

    size_t total = sizeof(sharp_object) + bytes;
    int c = (int)((total + 15) / 16) - 1;      // 32 bytes -> class 1, 48 -> 2 ...
    sharp_object *o;
    if(c <= POOL_CLASSES) {
        reserve_bytes(total, unsafe);
        o = (sharp_object*) pool_alloc(c);
    } else {
        o = (sharp_object*) malloc_mem<char>(total, unsafe);
        c = 0;
    }

    init_struct(o);
    o->poolClass = c;
    if(bytes > 0) {
        o->data = (void*)(o + 1);
        memset(o->data, 0, bytes);
        o->inlineData = 1;
    }
    return o;
}

sharp_object* create_static_object(sharp_class* sc, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), unsafe);

    init_struct(o);
    o->size = sc->staticFields;
    o->type = type_class;
    o->staticObject = 1;
    SET_INFO(o->info, sc->address, gc_normal);

    if(o->size > 0)
    {
        // every slot starts as zero bits: 0 for an integer field, 0.0 for a
        // decimal field, null for a reference (see "THE SLOTS OF A CLASS
        // INSTANCE" in sharp_object.h)
        o->node = calloc_mem<object>(o->size, sizeof(object), unsafe);
    }

    push_object(o);
    return o;
}

sharp_object* create_object(sharp_class* sc, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    auto o = new_object_with_payload(sizeof(object) * (size_t)sc->instanceFields, unsafe);

    o->size = sc->instanceFields;
    o->type = type_class;
    SET_INFO(o->info, sc->address, gc_normal);

    push_object(o);
    return o;
}

sharp_object* create_object(sharp_class* sc, Int size, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    if(size > 0)
    {
        auto o = new_object_with_payload(sizeof(object) * (size_t)size, unsafe);

        o->size = size;
        o->type = type_class;
        o->arrayFlag = 1;
        SET_INFO(o->info, sc->address, gc_normal);
        push_object(o);
        return o;
    }

    return nullptr;
}

void realloc_object(sharp_object *o, Int size, bool unsafe) {
    if(o) {
        if(size <= 0) {
            throw vm_exception("call to realloc() with size <= 0");
        }

        if(o->inlineData) {
            // the payload is part of the header's block and cannot
            // be resized: give it a block of its own first. The old bytes stay
            // where they are until the object dies; they are no longer counted.
            size_t bytes = (o->type <= type_var ? (size_t)element_size(o->type) : sizeof(object)) * o->size;
            char *moved = malloc_mem<char>(bytes, unsafe);
            memcpy(moved, o->data, bytes);
            release_bytes(bytes);
            o->data = moved;
            o->inlineData = 0;
        }

        if(o->type <= type_var) {
            // elements are element_size(type) bytes each; the new ones start as 0
            size_t oldBytes = (size_t)element_size(o->type) * o->size;
            size_t newBytes = (size_t)element_size(o->type) * size;
            o->data = realloc_mem<char>(o->data, newBytes, oldBytes, unsafe);

            if(newBytes > oldBytes)
                memset((char*)o->data + oldBytes, 0, newBytes - oldBytes);

        } else {
            o->node = realloc_mem<object>(o->node, sizeof(object) * size, o->size, unsafe);

            if(size > o->size) {
                for(Int i = o->size; i < size; i++) {
                    o->node[i].o = NULL;
                }
            }
        }

        o->size = size;
    }
}

Int sizeof_object(sharp_object *o) {
    double size =0;
    if(o != nullptr) {

        if(o->type <= type_var) {
            size += element_size(o->type) * o->size;
        } else {
            const bool *numbers = number_slots_of(o);
            for(uInt i = 0; i < o->size; i++) {
                if(numbers != nullptr && numbers[i]) continue;     // a number, counted with the slots below
                if(o->node[i].o != nullptr) {
                    size += sizeof_object(o->node[i].o);
                }
            }

            size += sizeof(object) * o->size;
        }

        size += sizeof(sharp_object);
    }

    return size;
}

sharp_object* create_object(Int size, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    if(size > 0)
    {
        auto o = new_object_with_payload(sizeof(object) * (size_t)size, unsafe);

        o->size = size;
        o->type = type_object;
        o->arrayFlag = 1;
        SET_GENERATION(o->info, gc_normal);
        push_object(o);
        return o;
    }

    return nullptr;
}

sharp_object* create_object(Int size, data_type type, bool unsafe) {
    TELEMETRY_COUNT(tm_objects_created)
    if(size > 0)
    {
        auto o = new_object_with_payload((size_t)size * element_size(type), unsafe);     // see element_size() in sharp_object.h

        o->size = size;
        o->type = type;
        o->arrayFlag = 1;
        SET_GENERATION(o->info, gc_normal);

        push_object(o);
        return o;
    }

    return nullptr;
}

/*
 * A number array of length 0 that IS an object (create_object() above gives
 * null for size 0, which is how a program's own `new _int8[0]` is held).
 * Needed only where the library insists on an object: platform.srt_init
 * turns every command-line argument into a string with `args[i] as _int8[]`,
 * and a cast of null raises nullptr_exception - so an empty argument
 * (`sharp prog ""`) stopped every program before main. `data` stays null: the
 * collector frees it with free(nullptr) and counts 0 bytes, and every access
 * is out of bounds, as it should be for length 0.
 *
 * C++ comparison: `new char[0]` - a real, empty array.
 */
sharp_object* create_empty_array(data_type type) {
    TELEMETRY_COUNT(tm_objects_created)
    auto o = malloc_mem<sharp_object>(sizeof(sharp_object), false);

    init_struct(o);
    o->size = 0;
    o->type = type;
    o->arrayFlag = 1;
    SET_GENERATION(o->info, gc_normal);
    o->data = nullptr;

    push_object(o);
    return o;
}

/*
 * SHARED TEXT: one object for a text literal, however often its line runs.
 *
 * `newstring s` makes a new _int8[] every time, because an array can be changed
 * and the next run of the line must see the text as written. Where the compiler
 * has proved that the text is only ever READ (never changed, never kept
 * anywhere, never compared by identity) it writes a NEGATIVE operand, and the
 * instruction gives the same object every time:
 *
 *      operand = -(2 * s + 1)      the shared _int8[] of string s
 *      operand = -(2 * s + 2)      the shared `string` object whose data is that array
 *
 *      if(name == "brian")         one array for the life of the program, not one per test
 *
 * The objects are made the first time they are asked for and are permanent
 * (never freed, and roots for the collector). An empty text has no array, as
 * with the plain instruction: null, or a string whose data is null.
 *
 *      C++ comparison: a string literal in C++ is one `const char[]` in the
 *      program's read-only data; Java keeps one String per literal (the pool).
 */
static std::mutex sharedTextMut;
static std::atomic<sharp_object*> *sharedArrays = nullptr, *sharedStrings = nullptr;   // per string of vm.strings
static std::atomic<bool> sharedTextReady{false};

sharp_object* shared_text(Int operand) {
    Int code = -operand - 1, index = code >> 1;
    bool wantsString = (code & 1) != 0;
    if(index < 0 || index >= (Int) vm.mf.strings)
        throw vm_exception("newstring: no string " + to_string((long long) index) + " in the string table");

    if(sharedTextReady.load(std::memory_order_acquire)) {
        sharp_object *made = (wantsString ? sharedStrings : sharedArrays)[index].load(std::memory_order_acquire);
        if(made != nullptr) return made;
    }

    std::lock_guard<std::mutex> guard(sharedTextMut);
    if(!sharedTextReady.load(std::memory_order_acquire)) {
        size_t count = (size_t) vm.mf.strings + 1;
        sharedArrays = new std::atomic<sharp_object*>[count];
        sharedStrings = new std::atomic<sharp_object*>[count];
        for(size_t i = 0; i < count; i++) { sharedArrays[i] = nullptr; sharedStrings[i] = nullptr; }
        sharedTextReady.store(true, std::memory_order_release);
    }

    string &text = vm.strings[index];
    sharp_object *array = sharedArrays[index].load(std::memory_order_acquire);
    if(array == nullptr && text.size() > 0) {
        array = create_object(text.size(), type_int8);
        assign_string_field(array, text);
        tgc_make_permanent(array);
        sharedArrays[index].store(array, std::memory_order_release);
    }
    if(!wantsString) return array;

    sharp_object *whole = sharedStrings[index].load(std::memory_order_acquire);
    if(whole == nullptr) {
        whole = create_object(vm.string_class);
        object *data = resolve_field("data", whole);
        if(data != nullptr) store_object(data, array);
        tgc_make_permanent(whole);
        sharedStrings[index].store(whole, std::memory_order_release);
    }
    return whole;
}

void cast_object(object *from, Int toClass) {
    if(!(from->o->type == type_class && is_type(from, toClass))) {
        stringstream ss;
        if(from->o->type == type_class) {
            ss << "invalid cast from: "
                  + vm.classes[CLASS(from->o->info)].fullName + " to: " + vm.classes[toClass].fullName;
        } else {
            auto clazz = vm.classes[CLASS(from->o->info)].fullName;
            switch(from->o->type) {
                case type_int8:
                    ss << "casting from _int8" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_int16:
                    ss << "casting from _int16" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_int32:
                    ss << "casting from _int32" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_int64:
                    ss << "casting from _int64" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint8:
                    ss << "casting from _uint8" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint16:
                    ss << "casting from _uint16" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint32:
                    ss << "casting from _uint32" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_uint64:
                    ss << "casting from _uint64" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_function_ptr:
                    ss << "casting from func_ptr*" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                case type_object:
                    ss << "casting from object" << (from->o->arrayFlag ? "[]" : "") << " to " << clazz;
                    break;
                default:
                    ss << "casted object does not match expected type";
                    break;
            }
        }

        vm_exception err(vm.class_cast_except, ss.str());   // a cast that cannot succeed
        enable_exception_flag(thread_self, true);
        throw err;
    }
}

void cast_numeric_array(object *from, Int toType) {
    if(!(from->o->type <= type_var && is_type(from, toType))) {
        stringstream ss;
        switch(from->o->type) {
            case type_int8:
                ss << "illegal cast from _int8" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_int16:
                ss << "illegal cast from _int16" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_int32:
                ss << "illegal cast from _int32" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_int64:
                ss << "illegal cast from _int64" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint8:
                ss << "illegal cast from _uint8" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint16:
                ss << "illegal cast from _uint16" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint32:
                ss << "illegal cast from _uint32" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_uint64:
                ss << "illegal cast from _uint64" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_function_ptr:
                ss << "illegal cast from func_ptr*" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_object:
                ss << "illegal cast from object" << (from->o->arrayFlag ? "[]" : "");
                break;
            case type_class: {
                auto clazz = vm.classes[CLASS(from->o->info)].fullName;
                ss << "illegal cast from class " << clazz;
                break;
            }
            default:
                ss << "illegal cast object does not match expected type";
                break;
        }

        vm_exception err(vm.class_cast_except, ss.str());   // a cast that cannot succeed
        enable_exception_flag(thread_self, true);
        throw err;
    }
}

// ---------------------------------------------------------------------------
// lock(x) { ... }
//
//   LOCK   -> lock_object(x)       at the start of the block
//   ULOCK  -> unlock_object(x)     at its end (the compiler also emits it in
//                                  front of a return / break / continue that
//                                  leaves the block)
//
// Two things a lock block has to get right, and did not:
//
//   1. Nesting. lock(x) { lock(x) { } ...rest... } - the same fiber may enter
//      a lock it already holds. The lock counts how often (fib_mutex::count)
//      and is only given back when the outermost block ends. It used to be
//      released by the first ULOCK, so "rest" ran unprotected.
//
//   2. Exceptions. An exception that leaves the block skips its ULOCK. Every
//      block a fiber is inside of is therefore written down (fiber::held, with
//      the call depth and pc of its LOCK); when an exception is caught further
//      out, or unwinds a function, the blocks it left are released by
//      release_locks_left_by_exception(). Before this the fiber kept the lock
//      until it ended.
//
// C++ comparison: std::lock_guard<std::recursive_mutex> - released by leaving
// the scope in any way, and re-entrant for the owner.
// ---------------------------------------------------------------------------

static void note_lock_held(fiber *task, fib_mutex *mut) {
    if(task->heldCount == task->heldCapacity) {
        Int capacity = task->heldCapacity == 0 ? 4 : task->heldCapacity * 2;
        held_lock *grown = (held_lock*)realloc(task->held, sizeof(held_lock) * capacity);
        if(grown == nullptr) return;    // out of memory: this block is simply not tracked
        task->held = grown;
        task->heldCapacity = capacity;
    }

    held_lock &entry = task->held[task->heldCount++];
    entry.mut = mut;
    entry.depth = task->callFramePtr;
    entry.pc = (Int)(task->pc - task->rom);
}

/** Forget the innermost block of `task` that holds `mut` (its ULOCK ran). */
static void forget_lock_held(fiber *task, fib_mutex *mut) {
    for(Int i = task->heldCount - 1; i >= 0; i--) {
        if(task->held[i].mut == mut) {
            for(Int j = i; j < task->heldCount - 1; j++)
                task->held[j] = task->held[j + 1];
            task->heldCount--;
            return;
        }
    }
}

/** One block less; the lock is free again when it was the last one. */
static void leave_lock(fiber *task, fib_mutex *mut) {
    if(mut->id != task->id) return;

    if(--mut->count > 0) return;    // still inside an outer lock(x) block

    mut->count = 0;
    mut->id = -1;

    // fibers that failed to take this lock are waiting to be told it is free
    sched_lock_released(mut);
}

void unlock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;

    if(mut->id == task->id) {
        forget_lock_held(task, mut);
        leave_lock(task, mut);
    }
}

/**
 * An exception is leaving code of the current fiber. Give back the locks of
 * every lock block it leaves:
 *
 *   - blocks entered in functions ABOVE `depth` (those functions are gone), and
 *   - blocks entered in the function at `depth` at a pc inside
 *     [fromPc, toPc] - the try block whose handler is about to run. Blocks
 *     nest, so a lock taken inside that try block also ends inside it.
 *
 * When the function at `depth` itself is unwound (nothing in it catches the
 * exception) the range is the whole function: fromPc = 0, toPc = max.
 *
 * Called by catch_exception() (virtual_machine.cpp), which both the
 * interpreter and the JIT use.
 */
void release_locks_left_by_exception(fiber *task, Int depth, Int fromPc, Int toPc) {
    for(Int i = task->heldCount - 1; i >= 0; i--) {
        held_lock &entry = task->held[i];

        if(entry.depth > depth
           || (entry.depth == depth && entry.pc >= fromPc && entry.pc <= toPc)) {
            fib_mutex *mut = entry.mut;

            for(Int j = i; j < task->heldCount - 1; j++)
                task->held[j] = task->held[j + 1];
            task->heldCount--;

            leave_lock(task, mut);
        }
    }
}

bool lock_object(sharp_object *o) {
    auto mut = create_mutex(o);
    auto task = thread_self->task;
    auto thread = thread_self;

    if(mut->id == task->id) {
        // already ours: one more block inside the same lock
        mut->count++;
        note_lock_held(task, mut);
        return false;
    } else {
        task->f_lock = mut;
    }

    lock_mut.lock();
    if(mut->id == -1) {
        mut->id = task->id;
        mut->count = 1;
        task->f_lock = nullptr;
        lock_mut.unlock();
        note_lock_held(task, mut);
        TELEMETRY_COUNT(tm_lock_acquired)
    } else {
        lock_mut.unlock();
        TELEMETRY_COUNT(tm_lock_contended)
        enable_context_switch(thread, true); // todo: check if in c++ env if so throw an exception
        return true;
    }

    return false;
}

extern void populate_string(string &s, sharp_object *o) {
    for(Int i = 0; i < o->size; i++)
        s += (unsigned char)integer_at(o, i);
}
