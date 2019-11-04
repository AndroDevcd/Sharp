//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../DataEntity.h"

class Param;
class Method;
class Expression;

class Field : public DataEntity {
public:
    Field()
        :
        DataEntity(),
        key(""),
        isArray(false),
        nullField(false),
        fnSignature(NULL),
        dvExpression(NULL),
        klass(NULL),
        locality(stl_stack)
    {
    }

    Field(DataType type, long guid, const string name, ClassObject* owner, List<AccessFlag>& flags,
           Meta& meta, StorageLocality stl, long addr)
        :
        DataEntity(),
        locality(stl),
        key(""),
        isArray(false),
        nullField(false),
        fnSignature(NULL),
        dvExpression(NULL),
        klass(NULL)
    {

        this->type = type;
        this->guid = guid;
        this->name = name;
        this->address = addr;
        this->owner = owner;
    }

    void free();
    bool equals(Field& f);

public:
    string key;
    bool isArray;
    bool nullField;
    StorageLocality locality;
    Method* fnSignature;
    Expression* dvExpression;
    ClassObject *klass;
};


#endif //SHARP_FIELD_H
