//
// Created by BraxtonN on 2/12/2018.
//

#include "ClassObject.h"
#include "Field.h"
#include "Object.h"

void ClassObject::free() {

    if(fields != NULL) {
        for(Int i = 0; i < totalFieldCount; i++)
            fields[i].free();
        std::free(fields); fields = NULL;
    }

    if(methods != NULL)
        std::free(this->methods);
    methods = NULL;
}

Field *ClassObject::getfield(string name) {
    for(unsigned int i = 0; i < totalFieldCount; i++) {
        if(fields[i].name == name)
            return &fields[i];
    }
    return NULL;
}

Int ClassObject::fieldindex(string name) {
    for(Int i = totalFieldCount-1; i >= 0; i--) {
        if(fields[i].name == name)
            return i;
    }
    return -1;
}

bool ClassObject::isClassRelated(ClassObject *k) {
    if(k == NULL)
        return false;
    if(this->guid==k->guid)
        return true;

    return super == NULL ? false : super->isClassRelated(k);
}
