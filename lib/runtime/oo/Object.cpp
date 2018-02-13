//
// Created by BraxtonN on 2/12/2018.
//

#include "Object.h"
#include "../memory/GarbageCollector.h"


void Object::operator=(Object &object) {
    GarbageCollector::self->freeObject(this);
    GarbageCollector::self->attachObject(this, object.object);
}
