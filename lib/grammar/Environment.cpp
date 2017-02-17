//
// Created by BraxtonN on 2/7/2017.
//

#include "Environment.h"
#include "runtime.h"

int_ClassObject* Environment::create_class(int_ClassObject obj) {
    for(int_ClassObject& o : *classes) {
        if(o.refrence->match(obj.refrence)) {
            return &o;
        }
    }

    classes->push_back(obj);
    return &element_at(*classes, classes->size() - 1);
}

void int_ClassObject::create(ClassObject *klass) {
    this->refrence = klass;

    for(int64_t i = 0; i < klass->functionCount(); i++) {
        this->functions->push_back(int_Method(klass->getFunction(i)));
    }

    for(int64_t i = 0; i < klass->constructorCount(); i++) {
        this->constructors->push_back(int_Method(klass->getConstructor(i)));
    }
}
