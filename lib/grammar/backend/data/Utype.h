//
// Created by braxtonn on 11/11/2019.
//

#ifndef SHARP_UTYPE_H
#define SHARP_UTYPE_H

#include "../../../../stdimports.h"
#include "DataEntity.h"
#include "../code/CodeHolder.h"

enum utype_struct
{
    utype_field,
    utype_class,
    utype_method,
    utype_function_ptr,
    utype_native,
    utype_literal,
    utype_unresolved
};

class Method;

/**
 * Injector for building code
 */
extern string stackInjector;
extern string ebxInjector;
extern string ptrInjector;
extern string getterInjector;
extern string removeFromStackInjector;
extern string incInjector;
extern string decInjector;

class Utype {
public:
    Utype()
    :
        code(),
        resolvedType(NULL),
        type(utype_unresolved),
        array(false),
        nullType(false)
    {
    }

    Utype(ClassObject* k, bool isArray = false);
    Utype(DataType type, bool isArray = false);

    void setType(utype_struct type) { this->type = type; }
    void setResolvedType(DataEntity* utype, bool shouldFree = false) { if(shouldFree) free(); resolvedType = utype; }
    utype_struct getType() { return type; }
    void setArrayType(bool isArray) { array = isArray; }
    bool isArray() { return array; }
    void setNullType(bool nt) { nullType = nt; }
    bool isNullType() { return nullType; }
    bool isClass();
    ClassObject* getClass();
    Method *getMethod();
    DataEntity* getResolvedType() { return resolvedType; }
    CodeHolder& getCode() { return code; }
    void free();
    string toString();
    bool equals(Utype *utype);
    bool isRelated(Utype *utype);
    void copy(Utype *utype) {
        type = utype->type;
        array = utype->array;
        nullType = utype->nullType;
        resolvedType = utype->resolvedType;
    }

private:
    utype_struct type;
    DataEntity *resolvedType;
    bool array;
    bool nullType;
    CodeHolder code;
};


#endif //SHARP_UTYPE_H
