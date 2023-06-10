//
// Created by bknun on 9/21/2022.
//

#include "reflect_helpers.h"
#include "../virtual_machine.h"
#include "../types/sharp_field.h"
#include "../memory/garbage_collector.h"
#include "../error/vm_exception.h"
#include "../../core/access_flag.h"

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
                if(isStatic == check_flag(field.flags, flag_static)) {
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

void assign_numeric_field(sharp_object* o, uInt index, double value) {
    o->HEAD[index] = value;
}

void assign_numeric_class_field(sharp_object* o, double value) {
    auto field = resolve_field("value", o);
    if(field) assign_numeric_field(field->o, 0, value);
}

void assign_string_field(sharp_object* o, string &value) {
    if(o->size == value.size()) {
        for(Int i = 0; i < value.size(); i++) {
            o->HEAD[i] = value[i];
        }
    } else {
        throw vm_exception("string.size != field's size");
    }
}

double read_numeric_value(sharp_object *o, Int index) {
    if(o) {
        if(o->type <= type_var) {
            if(index < o->size)
                return o->HEAD[index];
            else {
                stringstream ss;
                ss << "access to field at index: " << index << " with size of: " << o->size;
                throw vm_exception(vm.bounds_except, ss.str());
            }
        } else {
            stringstream ss;
            ss << "access to non-numeric field";
            throw vm_exception(vm.invalid_operation_except, ss.str());
        }
    }

    return 0;
}

string read_string_value(sharp_object *o) {
    string s;
    if(o != NULL) {
        if (o->type <= type_var) {
            for(Int i = 0; i < o->size; i++) {
                s += (char)o->HEAD[i];
            }
        }
    }
    return s;
}
