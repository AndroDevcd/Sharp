//
// Created by bknun on 5/28/2018.
//

#ifndef SHARP_GENERICTYPE_H
#define SHARP_GENERICTYPE_H

#include "../../stdimports.h"
#include "FieldType.h"

class ClassObject;

class GenericType {

public:

private:
    string generic_id;
    FieldType type;
    ClassObject* klass;
    bool array;

};


#endif //SHARP_GENERICTYPE_H
