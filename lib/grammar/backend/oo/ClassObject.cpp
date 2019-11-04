//
// Created by BraxtonN on 10/22/2019.
//

#include "ClassObject.h"
#include "../Compiler.h"


void ClassObject::free()
{
    release();
    Compiler::freeListPtr(constructors);
}

Field* ClassObject::getField(string& name, bool checkBase) {
    Field *field = NULL;
    for(long i = 0; i < fields.size(); i++) {
        field = fields.get(i);
        if(field->name == name) {
            return field;
        }
    }

    if(!field && checkBase)
        super->getField(name, true);

    return NULL;
}

Method* ClassObject::getConstructor(List<Field*> params, bool checkBase) {
    Method *fun;
    for(long i = 0; i < constructors.size(); i++) {
        fun = constructors.get(i);
        if(Compiler::simpleParameterMatch(params, fun->params)) {
            return fun;
        }
    }

    if(!fun && checkBase)
        super->getConstructor(params, true);

    return NULL;
}