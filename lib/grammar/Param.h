//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_PARAM_H
#define SHARP_PARAM_H

#include "Field.h"
#include "FieldType.h"

class ClassObject;

class Param
{
public:
    Param()
            :
            field()
    {
    }

    Param(Field field)
    {
        this->field = field;
    }

    bool match(Param& param);
    static bool match(List<Param>& p1, List<Param>& p2, bool stringSupport, bool ambiguousProtect);
    void free() {
        field.free();
    }


    Field field;
};
#endif //SHARP_PARAM_H
