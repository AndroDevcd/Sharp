//
// Created by BraxtonN on 2/2/2018.
//

#include "Param.h"
#include "ClassObject.h"

bool Param::match(Param &param) {
    return field == param.field;
}

bool Param::match(List<Param>& p1, List<Param>& p2, bool nativeSupport) {
    if(p1.size() != p2.size())
        return false;

    size_t iter = 0;
    for(unsigned int i = 0; i < p1.size(); i++) {
        if(!p1.get(i).match(p2.get(iter++))) {
            return (p1.get(i).field.type == CLASS
                   && p2.get(iter-1).field.type == CLASS
                   && p1.get(i).field.klass->assignable(p2.get(iter-1).field.klass))
                    || (nativeSupport && p1.get(i).field.type == CLASS && p1.get(i).field.klass->getModuleName() == "std"
                        && p1.get(i).field.klass->getName() == "string" && p2.get(iter-1).field.type == VAR
                        && p2.get(iter-1).field.isArray);
        }
    }

    return true;
}