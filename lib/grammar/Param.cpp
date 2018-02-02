//
// Created by BraxtonN on 2/2/2018.
//

#include "Param.h"

bool Param::match(Param &param) {

    return field == param.field;
}

bool Param::match(List<Param>& p1, List<Param>& p2) {
    if(p1.size() != p2.size())
        return false;

    size_t iter = 0;
    for(unsigned int i = 0; i < p1.size(); i++) {
        if(!p1.get(i).match(p2.get(iter++)))
            return false;
    }

    return true;
}