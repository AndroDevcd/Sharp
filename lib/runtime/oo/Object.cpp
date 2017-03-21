//
// Created by BraxtonN on 2/17/2017.
//

#include "Object.h"
#include "../internal/Environment.h"
#include "ClassObject.h"
#include "Array.h"

gc_object::gc_object() {
    this->mark = gc_orange;
    this->monitor = new Monitor();
    this->type = nilobject;
    this->HEAD = NULL;
    next = NULL, prev=NULL;
}

gc_object::gc_object(int64_t type) {
    this->mark = gc_orange;
    this->monitor = new Monitor();
    this->type = type;
    this->HEAD = NULL;
    next = NULL, prev=NULL;
}

void gc_object::free() {
    if(mark == gc_green) {
        mark = gc_orange;
    }
}

void gc_object::copy_object(gc_object *pObject) {
    // TODO: implement
}

void gc_object::createnative(int type, int64_t size) {
    if(mark != gc_green) {
        HEAD=new double[size];
        this->type=type;
        this->size=size;
        prev = NULL, next=NULL;

        for(int64_t i=0; i<size; i++){
            _nativewrite(i,0)
        }
    }
}
