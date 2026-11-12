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
struct object {
    sharp_object *o;
};

struct sharp_object {
    union {
        long double *HEAD;   /* data */
        object *node;        /* structured data */
    };

    /* info */
    uint32_t size;
    // (There used to be a reference count here. Objects are now freed by the
    //  tracing collector, memory/tracing_collector.h, and nothing is counted.)
    unsigned short type : 5; // the type of object represents {class, var, _int64, etc}
    unsigned short arrayFlag : 1;

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
inline void copy_object(object *to, object *from) {
    to->o = from ? from->o : nullptr;
}

inline void copy_object(object *to, sharp_object *from) {
    to->o = from;
}
sharp_object* create_object(sharp_class* sc, bool unsafe = false);
sharp_object* create_object(sharp_class* sc, Int size, bool unsafe = false);
sharp_object* create_static_object(sharp_class* sc, bool unsafe = false);
sharp_object* create_object(Int size, data_type type, bool unsafe = false);
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
