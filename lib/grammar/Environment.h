//
// Created by BraxtonN on 2/7/2017.
//

#ifndef SHARP_ENVIRONMENT_H
#define SHARP_ENVIRONMENT_H

#include "../../stdimports.h"
#include "OperatorOverload.h"
#include "../util/keypair.h"

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
    }

    void create_class(int_ClassObject obj);

private:
    std::list<int_ClassObject>* classes;
};

class int_ClassObject {
public:
    /* Create object from class */
    int_ClassObject()
    :
            name(""),
            module_name(""),
            uid(0)
    {
        constructors = new std::list<int_Method>();
        functions = new std::list<int_Method>();
        macros = new std::list<int_Method>();
        fields = new std::list<Field>();
        tmap = new std::list<ResolvedRefrence>();
    }

    int_ClassObject(ClassObject* klass)
    {
        create(klass);
    }

    void create(ClassObject* klass);

    long uid;
    bool tmpl;
    const string name;
    const string module_name;
    std::list<int_Method>* constructors;
    std::list<int_Method>* functions;
    std::list<int_Method>* macros;
    std::list<Field> *fields;
    std::list<ResolvedRefrence>* tmap;
    ClassObject* parent, *super;
    ClassObject* base;
};

class int_Method : public Method {
public:
    int_Method(const string &name, const string &module, ClassObject *klass, const list <Param> &params,
               const list <AccessModifier> &modifiers, ClassObject *rtype, const RuntimeNote &note);
    int_Method(string name, string module, ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
               NativeField rtype, RuntimeNote note);
private:
    _operator op;
    // code for storing executable bytecode

};


#endif //SHARP_ENVIRONMENT_H
