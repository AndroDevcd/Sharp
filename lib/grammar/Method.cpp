//
// Created by BraxtonN on 2/5/2018.
//
#include "Method.h"
#include "ClassObject.h"

string Method::getFullName(){
    return owner==NULL ? name : owner->getFullName() + "." + name;
}
