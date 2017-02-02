//
// Created by BraxtonN on 1/31/2017.
//
#include "Param.h"
#include "ClassObject.h"
#include "runtime.h"

bool Param::match(Param &param) {
    if(field == NULL || param.field == NULL)
        return false;

    return *field == *param.field;
}

bool Param::match(list<Param>& p1, list<Param>& p2) {
    if(p1.size() != p2.size())
        return false;

    size_t iter = 0;
    for(Param& param : p1) {
        if(!param.match(*std::next(p2.begin(), iter++)))
            return false;
    }

    return true;
}
