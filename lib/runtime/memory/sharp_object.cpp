//
// Created by bknun on 9/16/2022.
//

#include "sharp_object.h"


void init_struct(object *o) {
    o->o = nullptr;
}

void init_struct(sharp_object *o) {
    o->next = nullptr;
    o->type = 0;
    o->size = 0;
    o->info = 0;
    o->refCount = 0;
    o->HEAD = nullptr;
}
