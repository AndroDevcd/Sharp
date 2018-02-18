//
// Created by BraxtonN on 2/12/2018.
//

#include "Object.h"
#include "../memory/GarbageCollector.h"

CXX11_INLINE
void Object::operator=(Object &object) {
    GarbageCollector::self->freeObject(this);
    GarbageCollector::self->attachObject(this, object.object);
}

CXX11_INLINE
void Object::operator=(SharpObject *object, bool isNew) {
    GarbageCollector::self->freeObject(this);
    if(isNew)
        this->object = object;
    else
        GarbageCollector::self->attachObject(this, object);
}
