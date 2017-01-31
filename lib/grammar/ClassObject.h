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

class ClassObject {

public:
    ClassObject(string name, string pmodule, long uid, AccessModifier modifier)
    :
            name(name),
            module_name(pmodule),
            uid(uid),
            modifier(modifier),
            nf(fnof)
    {
        functions = new list<Method>();
        constructors = new list<Method>();
        fields = new list<Field>();
        childClasses = new list<ClassObject>();
    }

    ClassObject(string name, string pmodule, long uid, AccessModifier modifier, NativeField nf)
            :
            name(name),
            module_name(pmodule),
            uid(uid),
            nf(nf),
            modifier(modifier),
            functions(NULL),
            fields(NULL),
            childClasses(NULL),
            constructors(NULL)
    {
    }

    bool isNative() { return nf != fnof; }
    AccessModifier getAccessModifier() { return modifier; }
    long getUID() { return uid; }
    string getName() { return name; }
    string getModuleName() { return module_name; }
    bool match(ClassObject* klass) {
        return klass->uid == uid;
    }

    size_t constructorCount();
    Method* getConstructor(int p);
    Method* getConstructor(list<Param>& params);
    bool addConstructor(Method& constr);

    size_t functionCount();
    Method* getFunction(int p);
    Method* getFunction(string name, list<Param>& params);
    bool addFunction(Method &function);

    size_t fieldCount();
    Field* getField(int p);
    Field* getField(string name);
    bool addField(Field& field);

    size_t childClassCount();
    ClassObject* getChildClass(int p);
    ClassObject* getChildClass(string name);
    bool addChildClass(ClassObject& constr);

private:
    const NativeField nf;
    const AccessModifier modifier;
    const long uid;
    const string name;
    const string module_name;
    list<Method>* constructors;
    list<Method>* functions;
    list<Field> *fields;
    list<ClassObject>* childClasses;
};


#endif //SHARP_CLASSOBJECT_H
