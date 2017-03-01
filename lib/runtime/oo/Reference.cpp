//
// Created by BraxtonN on 2/28/2017.
//

#include "Reference.h"

void Reference::add(gc_object *obj) {
    obj->refs->push_back(this);
    object = obj;
}
