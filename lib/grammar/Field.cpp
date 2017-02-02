//
// Created by BraxtonN on 2/2/2017.
//
#include "Field.h"
#include "ClassObject.h"

bool Field::operator==(const Field& f)
{
    if(nf == fnof)
        return klass != NULL && klass->match(f.klass);
    else
        return (nf == f.nf) && name == f.name;
}