//
// Created by bknun on 9/18/2022.
//

#include <vector>
#include "sharp_class.h"
#include "sharp_field.h"
#include "sharp_type.h"
#include "../virtual_machine.h"
#include "../memory/garbage_collector.h"
#include "access_flag.h"


CXX11_INLINE void init_struct(sharp_class *sc) {
    sc->address = 0;
    sc->guid = 0;
    new (&sc->name) string();
    new (&sc->fullName) string();
    sc->owner = nullptr;
    sc->flags = 0; // todo: look into adding in classType for lower level inspecting
    sc->fields = nullptr;
    sc->methods = nullptr;
    sc->base = nullptr;
    sc->interfaces = nullptr;
    sc->staticFields = 0;
    sc->instanceFields = 0;
    sc->totalFieldCount = 0;
    sc->methodCount = 0;
    sc->interfaceCount = 0;
    sc->hash = 0;
    sc->instanceNumberSlots = nullptr;
    sc->staticNumberSlots = nullptr;
}

/** a field whose slot holds the number itself: a number that is not an array */
static bool is_number_field(const sharp_field &field) {
    return field.type != nullptr && field.type->type <= type_var && !field.isArray;
}

void build_number_slot_maps() {
    for(Int i = 0; i < vm.mf.classes; i++) {
        sharp_class *sc = vm.classes + i;

        // The fields of a class are stored instance fields first, then static
        // fields, each group in slot order: fields[n] describes instance slot
        // n, fields[instanceFields + n] describes static slot n.
        if(sc->instanceFields > 0) {
            sc->instanceNumberSlots = (bool*)calloc(sc->instanceFields, sizeof(bool));
            for(Int slot = 0; slot < sc->instanceFields; slot++)
                sc->instanceNumberSlots[slot] = is_number_field(sc->fields[slot]);
        }

        if(sc->staticFields > 0) {
            sc->staticNumberSlots = (bool*)calloc(sc->staticFields, sizeof(bool));
            for(Int slot = 0; slot < sc->staticFields; slot++) {
                const sharp_field &field = sc->fields[sc->instanceFields + slot];
                sc->staticNumberSlots[slot] = is_number_field(field) && check_flag(field.flags, flag_static);
            }
        }
    }
}

const bool* number_slots_of(const sharp_object *o) {
    if(o == nullptr || o->type != type_class || o->arrayFlag)
        return nullptr;

    const sharp_class *sc = vm.classes + CLASS(o->info);
    return o->staticObject ? sc->staticNumberSlots : sc->instanceNumberSlots;
}

/**
 * Can an object of class `base` be used where a `reference` is expected?
 * True when `base` IS `reference`, derives from it (through any number of
 * levels), or it - or one of its base classes - implements `reference` as an
 * interface (or an interface derived from it).
 *
 * In C++ terms: dynamic_cast<reference*>(base_object) != nullptr.
 *
 * This is what `as` (CAST), `is` (IS), catch clauses and the instance check of
 * a method call are decided with. It used to test each interface against
 * `base` itself instead of `reference`, which made every class that implements
 * an interface "related" to every other class: `a_dog as cat` raised nothing
 * and `a_dog is cat` was true.
 */
bool are_classes_related(sharp_class *base, sharp_class *reference) {
    if(base == NULL || reference == NULL)
        return false;

    // an interface of the class that has interfaces of its own (`interface b : a`): looked at
    // after the usual search, and only when there is one
    bool deeper = false;

    for(sharp_class *c = base; c != NULL; c = c->base) {
        if(c->guid == reference->guid)
            return true;

        // Each interface, and what that interface itself derives from.
        for(Int i = 0; i < c->interfaceCount; i++) {
            for(sharp_class *in = c->interfaces[i]; in != NULL; in = in->base) {
                if(in->guid == reference->guid)
                    return true;
                if(in->interfaceCount > 0) deeper = true;
                if(in->base == in) break;
            }
        }
        if(c->base == c) break;
    }

    if(!deeper)
        return false;

    // The interfaces OF the interfaces: `class c : b` with `interface b : a` is an `a` too.
    // Interface lists can refer back to each other (a recursive search ran until the stack
    // overflowed), so this keeps a list of what is still to be looked at and of what was seen.
    std::vector<sharp_class*> waiting, seen;
    for(sharp_class *c = base; c != NULL; c = c->base) {
        for(Int i = 0; i < c->interfaceCount; i++) waiting.push_back(c->interfaces[i]);
        if(c->base == c) break;
    }
    while(!waiting.empty()) {
        sharp_class *next = waiting.back();
        waiting.pop_back();
        for(sharp_class *in = next; in != NULL; in = in->base) {
            bool known = false;
            for(sharp_class *one : seen) { if(one == in) { known = true; break; } }
            if(known) break;
            seen.push_back(in);
            if(in->guid == reference->guid)
                return true;
            for(Int i = 0; i < in->interfaceCount; i++) waiting.push_back(in->interfaces[i]);
            if(in->base == in) break;
        }
    }
    return false;
}

thread_local std::hash<std::string> hasher;
uint32_t get_hash(string &name) {
    return hasher(name);
}