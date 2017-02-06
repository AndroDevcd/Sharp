//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H

#include <list>
#include "../../stdimports.h"
#include "Method.h"
#include "Field.h"
#include "NativeField.h"
#include "AccessModifier.h"
#include "OperatorOverload.h"
#include "../util/keypair.h"

class ClassObject {

public:
    ClassObject(string name, string pmodule, long uid, AccessModifier modifier, bool templ,
                list<string> templs, RuntimeNote note)
    :
            name(name),
            module_name(pmodule),
            uid(uid),
            modifier(modifier),
            parent(NULL),
            templClass(templ),
            base(NULL),
            note(note)
    {
        functions = new list<Method>();
        constructors = new list<Method>();
        macros = new list<Method>();
        overloads = new list<OperatorOverload>();
        fields = new list<Field>();
        childClasses = new list<ClassObject>();

        if(templ) {
            this->tmplRefs = new list<string>();
            *this->tmplRefs = templs;
        } else
            this->tmplRefs = NULL;
    }

    AccessModifier getAccessModifier() { return modifier; }
    long getUID() { return uid; }
    string getName() { return name; }
    string getModuleName() { return module_name; }
    bool match(ClassObject* klass) {
        return klass != NULL && klass->uid == uid;
    }
    void setBaseClass(ClassObject* base) {
        this->base = base;
    }
    list<string>* getTemplRefs() { return tmplRefs; }
    bool isTmplClass() { return templClass; }

    size_t constructorCount();
    Method* getConstructor(int p);
    Method* getConstructor(list<Param>& params);
    bool addConstructor(Method constr);

    size_t functionCount();
    Method* getFunction(int p);
    Method* getFunction(string name, list<Param>& params);
    bool addFunction(Method function);

    size_t overloadCount();
    OperatorOverload* getOverload(size_t p);
    OperatorOverload* getOverload(_operator op, list<Param>& params);
    bool addOperatorOverload(OperatorOverload overload);

    size_t fieldCount();
    Field* getField(int p);
    Field* getField(string name);
    bool addField(Field field);

    size_t macrosCount();
    Method* getMacros(int p);
    Method* getMacros(string name, list<Param>& params);
    bool addMacros(Method macros);

    size_t childClassCount();
    ClassObject* getChildClass(int p);
    ClassObject* getChildClass(string name);
    bool addChildClass(ClassObject constr);
    void free();

    RuntimeNote note;
private:
    const AccessModifier modifier;
    const long uid;
    const bool templClass;
    const string name;
    const string module_name;
    list<Method>* constructors;
    list<Method>* functions;
    list<Method>* macros;
    list<OperatorOverload>* overloads;
    list<Field> *fields;
    list<string> *tmplRefs;
    list<ClassObject>* childClasses;
    ClassObject* parent;
    ClassObject* base;
};


#endif //SHARP_CLASSOBJECT_H
