//
// Created by BraxtonN on 2/17/2017.
//

#include "ClassObject.h"
#include "../oo/Field.h"
#include "../oo/Object.h"
#include "../oo/Method.h"
#include "../internal/Environment.h"

void ClassObject::free() {
    if(fields != NULL) {
        for(int64_t i = 0; i < fieldCount; i++) {
            fields[i].free();
        }
        std::free(fields); fields = NULL;

        if(super != NULL) {
            super->free();
            std::free(super); super = NULL;
        }
    }

    if(flds != NULL) {
        for(int64_t i = 0; i < fieldCount; i++)
            flds[i].free();
        std::free(flds); flds = NULL;
    }

    if(methods != NULL) {
        for(int64_t i = 0; i < methodCount; i++)
            methods[i].free();
        std::free(methods); methods = NULL;
    }
}

ClassObject *ClassObject::newdup() {
    ClassObject* klass = new ClassObject(name.str(), flds, fieldCount,
                                         methods, methodCount, NULL,
                                         id);

    if(super != NULL) {
        klass->super = super->newdup();
    }
    if(fieldCount > 0) {
        klass->fields = new gc_object[fieldCount];
        for(int64_t i = 0; i < fieldCount; i++)
            klass->fields[i].type = (Type)flds[i].type;
    } else {
        klass->fields = NULL;
    }
    return klass;
}

gc_object *ClassObject::get_field(int64_t x) {
    if(fields == NULL)
        throw Exception(Environment::NullptrException, "");
    if(x >= fieldCount) {
        stringstream ss;
        ss << "illegal access to class field; invalid index " << x << ", size is " << fieldCount;
        throw Exception(Environment::IndexOutOfBoundsException, ss.str());
    }

    return &fields[x];
}
