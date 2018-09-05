//
// Created by BraxtonN on 2/12/2018.
//

#include "ClassObject.h"
#include "../oo/Field.h"
#include "../oo/Object.h"

void ClassObject::free() {

    if(fields != NULL) {
        for(int64_t i = 0; i < fieldCount; i++)
            fields[i].free();
        std::free(fields); fields = NULL;
    }

    if(methods != NULL)
        std::free(this->methods);
    methods = NULL;
    name.free();
}

Field *ClassObject::getfield(string name) {
    for(unsigned int i = 0; i < fieldCount; i++) {
        if(fields[i].name == name)
            return &fields[i];
    }
    return NULL;
}

int64_t ClassObject::fieldindex(string name) {
    for(long int i = fieldCount-1; i >= 0; i--) {
        if(fields[i].name == name)
            return i;
    }
    return -1;
}

void ClassObject::init() {
    this->name.init();
    fields = NULL;
    methods = NULL;
    super = NULL;
    serial = 0;
    fieldCount = 0;
    methodCount = 0;
}

bool ClassObject::hasBaseClass(ClassObject *pObject) {
    if(pObject == NULL)
        return false;
    if(this->serial==pObject->serial)
        return true;

    return base == NULL ? false : base->hasBaseClass(pObject);
}