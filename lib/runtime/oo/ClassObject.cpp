//
// Created by BraxtonN on 2/17/2017.
//

#include "ClassObject.h"
#include "../oo/Field.h"
#include "../oo/Object.h"
#include "../oo/Method.h"

void ClassObject::free() {
    if(fields != NULL) {
        for(int64_t i = 0; i < fieldCount; i++)
            fields[i].free();
        delete (fields); fields = NULL;

        if(super != NULL) {
            super->free();
            delete (super); super = NULL;
        }
    }

    if(flds != NULL) {
        for(int64_t i = 0; i < fieldCount; i++)
            flds[i].free();
        delete (flds); flds = NULL;
    }

    if(methods != NULL) {
        for(int64_t i = 0; i < methodCount; i++)
            methods[i].free();
        delete (methods); methods = NULL;
    }
}
