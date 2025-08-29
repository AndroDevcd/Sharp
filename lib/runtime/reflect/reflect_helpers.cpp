//
// Created by bknun on 9/21/2022.
//

#include "reflect_helpers.h"
#include "../virtual_machine.h"
#include "../types/sharp_field.h"
#include "../types/sharp_class.h"
#include "../memory/garbage_collector.h"
#include "../error/vm_exception.h"
#include "../../core/access_flag.h"

bool ignoreStaticResolution = false;

void ignore_static_resolve(bool ignore) {
    ignoreStaticResolution = ignore;
}

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
                if(ignoreStaticResolution || isStatic == check_flag(field.flags, flag_static)) {
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


object* instantiate_class_field_array(string& name, sharp_object *o, sharp_class *sc, Int size) {
    auto field = resolve_field(name, o);
    if(field != nullptr) {
        copy_object(field, create_object(sc, size));
        return field;
    }

    string className = vm.classes[CLASS(o->info)].fullName;
    throw vm_exception("could not find field `" + name + "` in class `" + className + "`");
}

object* instantiate_class_field(string& name, sharp_object *o, sharp_class *sc) {
    auto field = resolve_field(name, o);
    if(field != nullptr) {
        copy_object(field, create_object(sc));
        return field;
    }

    string className = vm.classes[CLASS(o->info)].fullName;
    throw vm_exception("could not find field `" + name + "` in class `" + className + "`");
}

sharp_class* locate_class(const char *name) {
    for(uInt i = 0; i < vm.mf.classes; i++) {
        if(vm.classes[i].fullName == name) {
            return &vm.classes[i];
        }
    }

    return nullptr;
}

sharp_function* locate_function(const char *name) {
    for(uInt i = 0; i < vm.mf.methods; i++) {
        if(vm.methods[i].fullName == name) {
            return &vm.methods[i];
        }
    }

    return nullptr;
}

sharp_class* locate_class_simple(const char *name) {
    for(uInt i = 0; i < vm.mf.classes; i++) {
        if(vm.classes[i].name == name) {
            return &vm.classes[i];
        }
    }

    return nullptr;
}

sharp_class* locate_class_hashed(uint32_t hash) {
    for(uInt i = 0; i < vm.mf.classes; i++) {
        if(vm.classes[i].hash == hash) {
            return &vm.classes[i];
        }
    }

    return nullptr;
}

void assign_numeric_field(sharp_object* o, uInt index, double value) {
    o->HEAD[index] = value;
}

void assign_object_field(sharp_object* o, uInt index, sharp_object* value) {
    copy_object(o->node + index, value);
}


void assign_string_class_field(sharp_object* o, string &value) {
    auto field = resolve_field("data", o);
    if(field) {
        if(!value.empty()) {
            copy_object(field, create_object(value.size(), type_int8));
            assign_string_field(field->o, value);
        } else {
            copy_object(field, (sharp_object*)nullptr);
        }
    }
}

object* assign_instance_to_class_field(string &name, sharp_object* o, sharp_object *instance) {
    auto field = resolve_field(name, o);
    if(field) {
        if(field->o != nullptr) {
            if(!IS_CLASS(field->o)) {
                throw vm_exception("reflect: assigning class instance to non-class field: " + name);
            } else if(CLASS(field->o->info) == CLASS(instance->info)) {
                copy_object(field, instance);
                return field;
            } else {
                auto sc = vm.classes + CLASS(instance->info);
                throw vm_exception("reflect: assigning incorrect class `" + sc->fullName + "` to field: " + name);
            }
        } else {
            copy_object(field, instance);
            return field;
        }
    }

    return nullptr;
}

void assign_numeric_class_field(sharp_object* o, double value) {
    auto field = resolve_field("value", o);
    if(field) assign_numeric_field(field->o, 0, value);
}

void assign_object_class_field(sharp_object* o, string &name, sharp_object* value) {
    auto field = resolve_field(name, o);
    if(field) copy_object(field, value);
}

void assign_string_field(sharp_object* o, string &value) {
    if(o != nullptr) {
        if (o->size == value.size()) {
            for (Int i = 0; i < value.size(); i++) {
                o->HEAD[i] = value[i];
            }
        } else {
            throw vm_exception("string.size != field's size");
        }
    } else {
        throw vm_exception("string field is null!");
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
