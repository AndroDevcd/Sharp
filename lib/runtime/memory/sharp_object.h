//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_SHARP_OBJECT_H
#define SHARP_SHARP_OBJECT_H

#include "../../../stdimports.h"
#include "../../core/data_type.h"

struct sharp_object;
struct sharp_class;

/**
 * Loose representation of an object if this object drops its
 * reference the reference lives on until the gc kills it
 */
/**
 * A reference to an object - and ALWAYS 8 BYTES, also where a pointer is 4 (a 32-bit CPU).
 *
 * The fields of a class instance are one block of 8-byte slots (see THE SLOTS OF A CLASS INSTANCE
 * below): a slot is a reference, or a number - a vm_cell, 8 bytes on every machine. The two kinds
 * sit side by side in one array and are found by index, so they must be the same size. `whole`
 * makes it so; nothing reads it.
 *
 *      32-bit:   [ o: 4 bytes ][ 4 unused ]        64-bit:   [ o: 8 bytes ]
 */
struct object {
    union {
        sharp_object *o;
        uint64_t whole;
    };
};
static_assert(sizeof(object) == 8, "a reference slot is 8 bytes on every machine: number fields share the array");
static_assert(sizeof(vm_cell) == 8, "a VM number is 8 bytes on every machine");

// (8: a payload stored right behind the header holds 8-byte numbers, also on a 32-bit CPU)
struct alignas(8) sharp_object {
    union {
        // A number array (type <= type_var: _int8[] .. decimal[], a string):
        // `size` elements of element_size(type) bytes each, packed. Read and
        // written with the typed array instructions (ALOAD8 ... ASTORE64) from
        // bytecode and with integer_at() / number_at() below from C++.
        void *data;
        object *node;        /* structured data */
    };

    /* info */
    uint32_t size;
    // (There used to be a reference count here. Objects are now freed by the
    //  tracing collector, memory/tracing_collector.h, and nothing is counted.)
    unsigned short type : 5; // the type of object represents {class, var, _int64, etc}
    unsigned short arrayFlag : 1;
    // this is the object that holds the STATIC fields of its class (one per
    // class, made at startup by create_static_object): its slots are described
    // by the class's static fields, not its instance fields
    unsigned short staticObject : 1;
    // `data` / `node` is not a block of its own: it is the memory
    // right behind this header, allocated with it (see new_object_with_payload)
    unsigned short inlineData : 1;
    // which size class of the object pool this block came from;
    // 0 = it came from malloc (see OBJECT POOL in sharp_object.cpp)
    unsigned short poolClass : 5;
    // 1 while this OLD object is in the collector's remembered set: a reference
    // was stored into it since the last collection, so it may point at a young
    // object (see THE WRITE BARRIER below). A byte of its own, not one of the
    // bits above: a thread sets it without a lock, and that must not disturb
    // the bits another thread may be changing.
    uint8_t remembered;

    /**
     * Information package
     *
     * This variable represent all of the crucial data for the object that is mainly used by the garbage collector
     * as well as the JIT runtime_old.
     *
     * It is important to understand that this type of information is the only type of data that can be packed into this
     * integer. The data should be treated as "read-only" data as the garbage collector should be the only thread to manipulate
     * and modify the contents of the information. Therefore allowing no mutex requirement to be involved when acting on the
     * packaged information.
     *
     * layout
     * 00000000 00000000 00000000 00000000 32 bits consisting of "class", "gc mark', 'generation', and 'lock'
     *
     *  low-end bits                                                                high-end bits
     *  0000 00000000 00000000 00000000   00              0           0
     *  ^-- class address                 ^-- generation  ^-- mark    ^--- lock
     */
    uint32_t info;
    sharp_object *next; /* linked list pointers */
};

/**
 * READING AND WRITING THE NUMBERS OF AN OBJECT FROM C++
 *
 * A number object (type <= type_var: a number array, a string, the box of a
 * numeric field) holds vm_cells that are ALL integers or ALL decimals, by its
 * element type: type_var is the only decimal one. C++ code that does not know
 * the type at compile time goes through these:
 *
 *      number_at(o, i)             the element as a double, whatever it is
 *      set_number_at(o, i, v)      store a double as what the object holds
 *      integer_at(o, i)            the element as an integer (characters, ids)
 *      set_integer_at(o, i, v)     store an integer as what the object holds
 *
 * Bytecode never needs them: the compiler knows the element type and emits an
 * instruction that reads the right member directly.
 */
inline bool holds_decimals(const sharp_object *o) { return o->type == type_var; }

/**
 * How many bytes one element of a number array of this type takes.
 *
 *      _int8  _uint8                        1      (a string is an _int8[])
 *      _int16 _uint16                       2
 *      _int32 _uint32                       4
 *      _int64 _uint64, function pointers    8
 *      decimal                              8      (a double)
 *
 * It used to be 16 bytes for every element (an 80-bit float), then 8.
 */
inline int element_size(int type) {
    switch(type) {
        case type_int8: case type_uint8: return 1;
        case type_int16: case type_uint16: return 2;
        case type_int32: case type_uint32: return 4;
        default: return 8;
    }
}

inline int64_t integer_at(const sharp_object *o, Int index) {
    switch(o->type) {
        case type_int8:   return ((int8_t*)o->data)[index];
        case type_uint8:  return ((uint8_t*)o->data)[index];
        case type_int16:  return ((int16_t*)o->data)[index];
        case type_uint16: return ((uint16_t*)o->data)[index];
        case type_int32:  return ((int32_t*)o->data)[index];
        case type_uint32: return ((uint32_t*)o->data)[index];
        case type_var:    return decimal_to_integer(((double*)o->data)[index]);
        default:          return ((int64_t*)o->data)[index];
    }
}

/** stores `value` cut to the element size, like a store from bytecode does */
inline void set_integer_at(sharp_object *o, Int index, int64_t value) {
    switch(o->type) {
        case type_int8: case type_uint8:   ((int8_t*)o->data)[index] = (int8_t)value; break;
        case type_int16: case type_uint16: ((int16_t*)o->data)[index] = (int16_t)value; break;
        case type_int32: case type_uint32: ((int32_t*)o->data)[index] = (int32_t)value; break;
        case type_var:                     ((double*)o->data)[index] = (double)value; break;
        default:                           ((int64_t*)o->data)[index] = value; break;
    }
}

inline double number_at(const sharp_object *o, Int index) {
    return holds_decimals(o) ? ((double*)o->data)[index] : (double)integer_at(o, index);
}

inline void set_number_at(sharp_object *o, Int index, double value) {
    if(holds_decimals(o)) ((double*)o->data)[index] = value;
    else set_integer_at(o, index, decimal_to_integer(value));
}

/**
 * THE SLOTS OF A CLASS INSTANCE
 *
 * An instance of a class (type_class, not an array) is one block of 8-byte
 * slots, `node`, one per field. What a slot holds depends on the field:
 *
 *      a number (not an array)     the number itself, a vm_cell   -> number_slot()
 *      anything else               a reference to an object       -> node[i].o
 *
 *      class point { x : decimal; y : decimal; label : string; }
 *      node:  [ x: 1.5 ][ y: 2.0 ][ label: -> string object ]
 *
 * A number field used to be a reference to a separate one element number
 * object: an allocation and a pointer hop for every numeric field.
 *
 *      C++ comparison: fields stored in the object, as in any C++ class.
 *
 * WHICH slots are numbers is a property of the class (number_slots_of() in
 * types/sharp_class.h). The garbage collector follows only the others. Reading
 * a number slot as a reference (node[i].o) or the reverse is a bug; from
 * bytecode the GETF / PUTF instructions are the only way to a number slot.
 *
 * Arrays are not affected: a class array or object array is all references,
 * a number array is HEAD.
 */
inline vm_cell& number_slot(sharp_object *o, Int index) {
    return *reinterpret_cast<vm_cell*>(o->node + index);
}

void init_struct(object *);
void init_struct(sharp_object *);

/**
 * Point the slot `to` at an object (or at nothing).
 *
 * This is ALL a reference assignment is: one pointer store. It used to take a
 * global lock and update two atomic reference counts (about 25ns each time);
 * the tracing collector needs neither, because it only looks at slots while
 * every thread is stopped at a safe point.
 *
 *      C++ comparison: assigning a raw pointer instead of a std::shared_ptr.
 */

/*
 * THE WRITE BARRIER (memory/tracing_collector.h, "GENERATIONS")
 *
 * A minor collection looks at young objects only. It finds them from the roots
 * - and from OLD objects that point at young ones, which it can only know
 * about if every store of a reference tells it. That is the write barrier:
 * the little extra work beside a store.
 *
 *      store_object(slot, value)       the store alone. For slots the collector
 *                                      scans anyway: a fiber's stack, a field of
 *                                      the fiber or the thread.
 *      write_barrier(owner, value,     `value` was stored into `slot`, a slot of
 *                    slot)             the object `owner`: if owner is old and
 *                                      value young, the next minor collection is
 *                                      told where to look. For most objects that
 *                                      is "all of owner, once" (owner->remembered).
 *                                      For a LARGE ARRAY it is the one slot: a
 *                                      list of a million elements that gets one
 *                                      new element must not be read through a
 *                                      million times.
 *      copy_object(slot, value)        for C++ code that does not know which
 *                                      object a slot belongs to: stores, and
 *                                      when the value is young and the slot is
 *                                      not on the running fiber's stack, keeps
 *                                      the VALUE alive through the next minor
 *                                      collection. Never wrong, sometimes keeps
 *                                      an object one collection too long.
 *
 *      C++ comparison: none. Java, C#, Go, Ruby and Lua all have one.
 */
#define OBJECT_GENERATION(o) (((o)->info >> 28) & 0x3u)     // 0 = young (garbage_collector.h: GENERATION)

// an array with more elements than this is remembered slot by slot, not as a whole
#define LARGE_ARRAY_SLOTS 128

void gc_remember(sharp_object *owner);                  // tracing_collector.cpp
void gc_remember_store(sharp_object *owner, object *slot);
void gc_remember_range(sharp_object *owner, Int first, Int count);
void gc_keep_young(object *slot, sharp_object *value);  // tracing_collector.cpp

inline void store_object(object *to, sharp_object *from) {
    to->o = from;
}

inline void store_object(object *to, object *from) {
    to->o = from ? from->o : nullptr;
}

inline void write_barrier(sharp_object *owner, sharp_object *value, object *slot) {
    if(value != nullptr && owner != nullptr && OBJECT_GENERATION(value) == 0
       && OBJECT_GENERATION(owner) != 0 && !owner->remembered)
        gc_remember_store(owner, slot);
}

/** `count` references were stored into `owner` from element `first` on, by a block copy */
inline void write_barrier_range(sharp_object *owner, Int first, Int count) {
    if(owner != nullptr && OBJECT_GENERATION(owner) != 0 && !owner->remembered)
        gc_remember_range(owner, first, count);
}

inline void copy_object(object *to, sharp_object *from) {
    to->o = from;
    if(from != nullptr && OBJECT_GENERATION(from) == 0)
        gc_keep_young(to, from);
}

inline void copy_object(object *to, object *from) {
    copy_object(to, from ? from->o : (sharp_object*) nullptr);
}
void pool_free_swept(sharp_object *o);     // called by the sweep for every dead object
void pool_sweep_done();                    // ... and once when the sweep is over
sharp_object* create_object(sharp_class* sc, bool unsafe = false);
sharp_object* create_object(sharp_class* sc, Int size, bool unsafe = false);
sharp_object* create_static_object(sharp_class* sc, bool unsafe = false);
sharp_object* create_object(Int size, data_type type, bool unsafe = false);
sharp_object* create_empty_array(data_type type);
sharp_object* shared_text(Int operand);             // the shared object of a text literal (SHARED TEXT in sharp_object.cpp)   // length 0, but an object (an empty command-line argument)
sharp_object* create_object(Int size, bool unsafe = false);
void realloc_object(sharp_object *, Int size, bool unsafe = false);
Int sizeof_object(sharp_object *o);
void cast_object(object *from, Int toClass);
void cast_numeric_array(object *from, Int toType);
bool lock_object(sharp_object *o);
class fiber;
void release_locks_left_by_exception(fiber *task, Int depth, Int fromPc, Int toPc);
void unlock_object(sharp_object *o);
void populate_string(string &s, sharp_object *o);


#endif //SHARP_SHARP_OBJECT_H
