//
// Created by BraxtonN on 2/1/2018.
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
#include "List.h"

class ClassObject {

public:
    ClassObject(string name, string pmodule, long serial, AccessModifier modifier, RuntimeNote note)
            :
            name(name),
            module_name(pmodule),
            serialId(serial),
            modifier(modifier),
            base(NULL),
            super(NULL),
            note(note)
    {
        functions.init();
        constructors.init();
        overloads.init();
        fields.init();
        childClasses.init();
    }

    AccessModifier getAccessModifier() { return modifier; }
    long getSerial() { return serialId; }
    string getName() { return name; }
    string getModuleName() { return module_name; }
    ClassObject* getSuperClass() { return super; }
    ClassObject* getBaseClass() { return base; }
    bool match(ClassObject* klass) {
        return klass != NULL && klass->uid == uid;
    }
    void setBaseClass(ClassObject* base) {
        this->base = base;
    }
    void setSuperClass(ClassObject* sup) {
        this->super = sup;
    }

    size_t constructorCount();
    Method* getConstructor(int p);
    Method* getConstructor(List<Param>& params);
    bool addConstructor(Method constr);

    size_t functionCount();
    Method* getFunction(int p);
    Method* getFunction(string name, List<Param>& params);
    bool addFunction(Method function);

    size_t overloadCount();
    OperatorOverload* getOverload(size_t p);
    OperatorOverload* getOverload(_operator op, List<Param>& params);
    bool addOperatorOverload(OperatorOverload overload);

    size_t fieldCount();
    Field* getField(int p);
    Field* getField(string name);
    bool addField(Field field);

    size_t childClassCount();
    ClassObject* getChildClass(int p);
    ClassObject* getChildClass(string name);
    bool addChildClass(ClassObject constr);
    void free();

    RuntimeNote note;

    bool isCurcular(ClassObject *pObject);

    bool matchBase(ClassObject *pObject);

private:
    const AccessModifier modifier;
    const long serialId;
    const string name;
    const string module_name;
    List<Method> constructors;
    List<Method> functions;
    List<OperatorOverload> overloads;
    List<Field> fields;
    List<ClassObject> childClasses;
    ClassObject *super, *base;
};


#endif //SHARP_CLASSOBJECT_H
