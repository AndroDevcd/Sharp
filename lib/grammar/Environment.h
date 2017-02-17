//
// Created by BraxtonN on 2/7/2017.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H

#include "../../stdimports.h"
#include "OperatorOverload.h"
#include "../util/keypair.h"
#include "BytecodeStream.h"

class Field;
class ClassObject;
class Method;
class int_ClassObject;
class int_Method;
class ResolvedRefrence;

class Environment {

public:
    Environment()
    {
        classes = new std::list<int_ClassObject>();
        macros = new std::list<int_Method>();
    }

    int_ClassObject* create_class(int_ClassObject obj);

private:
    std::list<int_ClassObject>* classes;
    std::list<int_Method>* macros;
};

class int_ClassObject {
public:
    /* Create object from class */
    int_ClassObject()
    {
        constructors = new std::list<int_Method>();
        functions = new std::list<int_Method>();
    }

    int_ClassObject(ClassObject* klass)
    {
        constructors = new std::list<int_Method>();
        functions = new std::list<int_Method>();

        create(klass);
    }

    void create(ClassObject* klass);

    std::list<int_Method>* constructors;
    std::list<int_Method>* functions;
    ClassObject* refrence;
};

class int_Method {
public:
    int_Method(Method* ref)
    :
            refrence(ref),
            op(op_NO)
    {
    }

    int_Method(Method* ref, _operator op)
            :
            refrence(ref),
            op(op)
    {
    }

    BytecodeStream bytecode;
private:
    Method* refrence;
    _operator op;
};


#endif //SHARP_ENVIRONMENT_H
