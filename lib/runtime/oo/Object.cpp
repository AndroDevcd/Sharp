//
// Created by BraxtonN on 2/17/2017.
//

#include "Object.h"
#include "../internal/Environment.h"
#include "ClassObject.h"
#include "Array.h"

gc_object::gc_object() {
    this->klass = Environment::nilClass;
    this->arry = Environment::nilArray;
    this->mark = gc_orange;
    this->obj = Environment::nilObject;
    this->refCounter = 0;
    this->type = nilobject;
}

gc_object::gc_object(Type type) {
    this->klass = Environment::nilClass;
    this->arry = Environment::nilArray;
    this->mark = gc_orange;
    this->obj = Environment::nilObject;
    this->refCounter = 0;
    this->type = type;
}

void gc_object::free() {
    if(mark == gc_green) {
        mark = gc_orange;
        switch (type) {
            case nativeint:
            case nativeshort:
            case nativelong:
            case nativechar:
            case nativebool:
            case nativefloat:
            case nativedouble: {
                if(obj != Environment::nilObject) {
                    delete (obj);
                }
                break;
            }
            case classobject: {
                if(klass != Environment::nilClass) {
                    klass->free();
                    delete (klass);
                }
                break;
            }
            case arrayobject: {
                if(arry != Environment::nilArray) {
                    arry->free();
                    delete (arry);
                }
                break;
            }
            default:
                break;
        }
    }
}
