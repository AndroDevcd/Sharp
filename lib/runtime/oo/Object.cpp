//
// Created by BraxtonN on 2/17/2017.
//

#include "Object.h"
#include "../internal/Environment.h"
#include "ClassObject.h"
#include "Array.h"

Sh_object::Sh_object() {
    this->mark = gc_orange;
    this->monitor = Monitor();
    this->type = nilobject;
    this->HEAD = NULL;
    next = NULL, prev=NULL;
}

Sh_object::Sh_object(int64_t type) {
    this->mark = gc_orange;
    this->monitor = Monitor();
    this->type = type;
    this->HEAD = NULL;
    next = NULL, prev=NULL;
}

void Sh_object::free() {
    if(mark == gc_green) {
        mark = gc_orange;
        if(HEAD != NULL)
            std::free(HEAD); HEAD = NULL;
    }
}

void Sh_object::copy_object(Sh_object *pObject) {
    // TODO: implement
}

void Sh_object::createnative(int type, int64_t size) {
    if(mark != gc_green) {
        HEAD= (double*)malloc(sizeof(double)*size);
        this->type=type;
        this->size=size;
        prev = NULL, next=NULL;
        mark = gc_green;

        for(int64_t i=0; i<size; i++){
            _nativewrite(i,0)
        }
    }
}
