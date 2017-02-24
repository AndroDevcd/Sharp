//
// Created by BraxtonN on 2/17/2017.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H

#include "../../../stdimports.h"

class Field;
class gc_object;
class Method;

class ClassObject {
public:
    string name;
    Field* flds;
    gc_object* fields;
    Method* methods;
    ClassObject* super;
    int64_t id, fieldCount, methodCount;
};


#endif //SHARP_CLASSOBJECT_H
