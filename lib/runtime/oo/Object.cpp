//
// Created by BraxtonN on 2/17/2017.
//

#include "Object.h"
#include "../internal/Environment.h"
#include "ClassObject.h"
#include "Array.h"
#include "Reference.h"

gc_object::gc_object() {
    this->klass = Environment::nilClass;
    this->arry = Environment::nilArray;
    this->mark = gc_orange;
    this->obj = Environment::nilObject;
    this->ref = Environment::nilReference;
    this->refCount = 0;
    this->monitor = new Monitor();
    this->type = nilobject;
}

gc_object::gc_object(Type type) {
    this->klass = Environment::nilClass;
    this->arry = Environment::nilArray;
    this->mark = gc_orange;
    this->obj = Environment::nilObject;
    this->ref = Environment::nilReference;
    this->refCount = 0;
    this->monitor = new Monitor();
    this->type = type;
}

void gc_object::free() {
    if(mark == gc_green) {
        mark = gc_orange;

        if(type <=nativedouble) {
            if(obj != Environment::nilObject) {
                std::free(obj);
                obj = Environment::nilObject;
            }
        }
        else if(type == classobject) {
            if(klass != Environment::nilClass) {
                klass->free();
                std::free(klass);
                klass = Environment::nilClass;
            }

        }
        else if(type == refrenceobject) {
            if(ref != Environment::nilReference) {
                if(ref->get_unsafe() != NULL) {
                    ref->get_unsafe()->inv_reference(ref);
                }
                std::free (ref);
                ref = Environment::nilReference;
            }

        } else {
            if(arry != Environment::nilArray) {
                arry->free();
                std::free(arry);
                arry = Environment::nilArray;
            }
        }
    }
}

void gc_object::invalidate() {
    int iter=0;
    for(Reference* refrence : this->refs) {
        if(iter++ >= refCount) break;
        refrence->notify();
    }
}

void gc_object::inv_reference(Reference *pReference) {

    int iter=0;
    for(Reference* refrence : this->refs) {
        if(iter++ >= refCount) break;
        if (refrence == pReference) {
            refs[iter] = NULL;
            return;
        }
    }
}

void gc_object::copy_object(gc_object *pObject) {
    // TODO: implement
}
