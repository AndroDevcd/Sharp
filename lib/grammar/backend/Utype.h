//
// Created by braxtonn on 11/11/2019.
//

#ifndef SHARP_UTYPE_H
#define SHARP_UTYPE_H

#include "../../../stdimports.h"
#include "DataEntity.h"
#include "IrCode.h"

enum utype_struct
{
    utype_field,
    utype_class,
    utype_method,
    utype_method_prototype,
    utype_native,
    utype_literal,
    utype_unresolved
};

extern string stackInjector;
extern string fnPtrInjector;
class Utype {
public:
    Utype()
    :
        code(),
        resolvedType(NULL),
        type(utype_unresolved),
        array(false),
        assignable(false)
    {
        code.addinjector(stackInjector);
    }

    Utype(ClassObject* k, bool isArray = false);
    Utype(DataType type, bool isArray = false);

    void setType(utype_struct type) { this->type = type; }
    void setResolvedType(DataEntity* utype, bool shouldFree = false) { if(shouldFree) free(); resolvedType = utype; }
    utype_struct getType() { return type; }
    void setArrayType(bool isArray) { array = isArray; }
    bool isArray() { return array; }
    void setAssignable(bool isAassignable) { assignable = isAassignable; }
    bool isAassignable() { return assignable; }
    bool isClass();
    ClassObject* getClass();
    DataEntity* getResolvedType() { return resolvedType; }
    IrCode& getCode() { return code; }
    void free();
    void softFree();
    string toString();
    bool equals(Utype *utype);

private:
    utype_struct type;
    DataEntity *resolvedType;
    bool array, assignable; // assignable is only used for utype_native and class utypes due to functions returning values we allow the compiler to interporet that as a value that can be digested into a field
    IrCode code;
};


#endif //SHARP_UTYPE_H
