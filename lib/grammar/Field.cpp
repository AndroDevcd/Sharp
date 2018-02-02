//
// Created by BraxtonN on 2/2/2018.
//

#include "Field.h"
#include "ClassObject.h"

bool Field::operator==(Field& f)
{
    if(f.type == this->type && f.array == this->array) {
        if(type == CLASS)
            return f.nullType || (klass != NULL && klass->match(f.klass));
        else
            return true;
    }
    return false;
}