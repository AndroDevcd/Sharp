//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../DataEntity.h"

class Param;
class Method;
class Expression;
class Utype;

class Field : public DataEntity {
public:
    Field()
        :
        DataEntity(),
        key(""),
        isArray(false),
        nullField(false),
        dvExpression(NULL),
        utype(NULL),
        locality(stl_stack),
        inlineCheck(false)
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
        dvExpression(NULL),
        utype(NULL),
        inlineCheck(false)
    {
        this->type = type;
        this->guid = guid;
        this->name = name;
        this->address = addr;
        this->owner = owner;
    }

    void free();
    bool equals(Field& f);
    bool isRelated(Field &f);
    bool isEnum();
    string toString();

public:
    string key;
    bool isArray;
    bool nullField;
    bool inlineCheck;
    StorageLocality locality;
    Expression* dvExpression;
    Utype *utype;
};


#endif //SHARP_FIELD_H
