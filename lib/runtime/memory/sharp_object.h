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
    uint32_t refCount;
    unsigned short type : 5; // the type of object represents {class, var, _int64, etc}

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
CXX11_INLINE void copy_object(object *to, object *from);
CXX11_INLINE void copy_object(object *to, sharp_object *from);
sharp_object* create_object(sharp_class* sc, bool unsafe = false);
sharp_object* create_object(Int size, data_type type, bool unsafe = false);
void lock_object(sharp_object *o);
void unlock_object(sharp_object *o);

#define inc_ref(x) (x)->refCount++;
#define dec_ref(x) (x)->refCount--;
#define invalid_references (-1)

#endif //SHARP_SHARP_OBJECT_H
