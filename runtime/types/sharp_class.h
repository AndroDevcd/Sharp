//
// Created by bknun on 9/18/2022.
//

#ifndef SHARP_SHARP_CLASS_H
#define SHARP_SHARP_CLASS_H

#include "../stdimports.h"
#include "data_type.h"

struct sharp_field;
struct sharp_function;
struct sharp_object;

struct sharp_class {
    int32_t address;
    int32_t guid;
    uint32_t hash;
    string name;
    string fullName;
    sharp_class *owner;
    uint32_t flags;
    sharp_field *fields;
    sharp_function **methods;
    sharp_class *base;
    sharp_class **interfaces;
    Int staticFields;
    Int instanceFields;
    Int totalFieldCount;
    Int methodCount;
    Int interfaceCount;

    // Which slots of an instance / of the static object hold a number instead
    // of a reference (see "THE SLOTS OF A CLASS INSTANCE" in
    // memory/sharp_object.h). One entry per slot, built by
    // build_number_slot_maps() once the classes are loaded.
    bool *instanceNumberSlots;
    bool *staticNumberSlots;
};

CXX11_INLINE void init_struct(sharp_class *sc);
bool are_classes_related(sharp_class*, sharp_class*);

/** Fill in instanceNumberSlots / staticNumberSlots of every class. Called once at startup. */
void build_number_slot_maps();

/**
 * For an object whose slots are described by a class (a class instance or a
 * class's static object): one bool per slot, true = the slot holds a number.
 * nullptr for every other object - arrays and plain objects hold references
 * in every slot.
 */
const bool* number_slots_of(const sharp_object *o);
uint32_t get_hash(string &name);

#endif //SHARP_SHARP_CLASS_H
