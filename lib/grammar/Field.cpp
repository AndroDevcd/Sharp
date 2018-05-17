//
// Created by BraxtonN on 2/2/2018.
//

#include "Field.h"
#include "ClassObject.h"

bool Field::operator==(Field& f)
{
    if(f.type == this->type && f.isArray == this->isArray) {
        if(type == CLASS)
            return f.nullType || (klass != NULL && klass->match(f.klass));
        else
            return true;
    } else if(f.nullType && !isArray){
        return type == CLASS || type==OBJECT;
    } else if(f.nullType && isArray){
        return type == CLASS || type==OBJECT || type == VAR;
    }
    return false;
}