//
// Created by BraxtonN on 10/22/2019.
//

#include "Field.h"
#include "../Compiler.h"
#include "ClassObject.h"
#include "Method.h"

void Field::free() {
    release();
    if(fnSignature)
    {
        fnSignature->free();
        delete fnSignature;
    }

//    if(dvExpression)
//    {
//        dvExpression->free();
//        delete dvExpression;
//    }
}

bool Field::equals(Field &f) {
    if(f.type == this->type && f.isArray == this->isArray) {
        if(type == CLASS)
            return (klass != NULL && klass->match(f.klass));
        else {
            if(type==FNPTR) {
                return Compiler::simpleParameterMatch(this->fnSignature->params, f.fnSignature->params);
            }
            return true;
        }
    }
    return false;
}