//
// Created by bknun on 9/18/2022.
//

#include "sharp_type.h"
#include "../memory/sharp_object.h"
#include "../memory/garbage_collector.h"
#include "../virtual_machine.h"

CXX11_INLINE void init_struct(sharp_type *st) {
    st->type = type_undefined;
    st->fun = nullptr;
    st->sc = nullptr;
}

bool is_type(object *o, Int type) {
    if(o && o->o) {
        if(o->o->type < type_var) {
            return o->o->type == type;
        } else if(o->o->type == type_class) {
            auto objClass = vm.classes + CLASS(o->o->info);
            auto *referenceClass = vm.classes + type;
            return are_classes_related(objClass, referenceClass);
        }
    }

    return false;
}
