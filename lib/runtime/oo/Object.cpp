//
// Created by BraxtonN on 2/17/2017.
//

#include "Object.h"
#include "../internal/Environment.h"
#include "ClassObject.h"

Sh_object::Sh_object() {
    this->mark = gc_orange;
    this->monitor = Monitor();
    this->type = nilobject;
    this->HEAD = NULL;
    nxt = NULL, prev=NULL;
    _Node = NULL;
    refs.init();
}

Sh_object::Sh_object(int64_t type) {
    this->mark = gc_orange;
    this->monitor = Monitor();
    this->type = type;
    this->HEAD = NULL;
    nxt = NULL, prev=NULL;
    _Node = NULL;
    refs.init();
}

void Sh_object::free() {
    mark = gc_orange;
    if(HEAD != NULL)
        std::free(HEAD); HEAD = NULL;

    for(unsigned long i=0; i < refs.size(); i++) {
        Sh_InvRef((*refs.at(i)));
    }

    size=0;
    refs.free();
}

void Sh_object::copy_object(Sh_object *pObject) {
    // TODO: implement
}

void Sh_object::createnative(int type, int64_t size) {
    if(mark != gc_green) {
        HEAD= (double*)malloc(sizeof(double)*size);
        this->type=type;
        this->size=size;
        prev = NULL, nxt=NULL;
        _Node=NULL, _rNode=NULL;
        mark = gc_green;

        for(int64_t i=0; i<size; i++){
            _nativewrite(i,0)
        }
    }
}

void Sh_object::_Sh_IncRef(Sh_object* o, Sh_object **ptr) {
    o->refs.push_back(ptr);
    (*ptr)->HEAD=o->HEAD;
    (*ptr)->nxt==o->nxt;
    (*ptr)->prev=o->prev;
    (*ptr)->size=o->size;
    (*ptr)->_Node=o->_Node;
    (*ptr)->type=o->type;
    (*ptr)->_rNode=o;
}
