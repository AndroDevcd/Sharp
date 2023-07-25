//
// Created by bknun on 9/18/2022.
//

#include "sharp_class.h"


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
}

bool are_classes_related(sharp_class *base, sharp_class *reference) {
    if(base == NULL || reference == NULL)
        return false;
    if(base->guid == reference->guid)
        return true;

    for(Int i = 0; i < base->interfaceCount; i++) {
        if(are_classes_related(base->interfaces[i], base)) {
            return true;
        }
    }

    return base->base != NULL && are_classes_related(base->base, reference);
}