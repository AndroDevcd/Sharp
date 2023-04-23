//
// Created by bknun on 9/21/2022.
//

#include "reflect_helpers.h"
#include "../virtual_machine.h"
#include "../types/sharp_field.h"
#include "../memory/garbage_collector.h"
#include "../../runtime_old/symbols/Field.h"

bool is_static_class(sharp_object *object) {
    return object != NULL && GENERATION(object->info) == gc_perm;
}

object* resolve_field(string name, sharp_object* o) {
    if(o && IS_CLASS(o)) {
        sharp_class *representedClass = &vm.classes[CLASS(o->info)];
        bool isStatic = is_static_class(o);

        for(Int i = 0; i < representedClass->totalFieldCount; i++) {
            sharp_field &field = representedClass->fields[i];
            if(field.name == name) {
                if(isStatic == IS_STATIC(field.flags)) {
                    return &o->node[field.address];
                }
                else {
                    return nullptr;
                }
            }
        }
    }

    return nullptr;
}

sharp_class* locate_class(const char *name) {
    for(uInt i = 0; i < vm.manifest.classes; i++) {
        if(vm.classes[i].fullName == name) {
            return &vm.classes[i];
        }
    }

    return nullptr;
}
