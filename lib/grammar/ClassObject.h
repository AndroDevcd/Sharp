//
// Created by BraxtonN on 2/1/2018.
//

#ifndef SHARP_CLASSOBJECT_H
#define SHARP_CLASSOBJECT_H

#include <list>
#include "../../stdimports.h"
#include "Method.h"
#include "Field.h"
#include "AccessModifier.h"
#include "OperatorOverload.h"
#include "../util/KeyPair.h"

class ClassObject {

public:
    ClassObject()
            :
            name(""),
            module_name(""),
            serial(0),
            modifier(),
            base(NULL),
            super(NULL),
            head(NULL),
            note(),
            fullName(""),
            address(-1)
    {
        functions.init();
        constructors.init();
        overloads.init();
        fields.init();
        childClasses.init();
    }
    ClassObject(string name, string pmodule, long uid, AccessModifier modifier, RuntimeNote note)
            :
            name(name),
            module_name(pmodule),
            serial(uid),
            modifier(modifier),
            base(NULL),
            super(NULL),
            head(NULL),
            note(note),
            fullName(""),
            address(-1)
    {
        functions.init();
        constructors.init();
        overloads.init();
        fields.init();
        childClasses.init();
    }

    ClassObject(string name, string pmodule, long uid, AccessModifier modifier, RuntimeNote note,
                ClassObject* parent)
            :
            name(name),
            module_name(pmodule),
            serial(uid),
            modifier(modifier),
            base(NULL),
            head(NULL),
            super(parent),
            note(note),
            fullName(""),
            address(-1)
    {
        functions.init();
        constructors.init();
        overloads.init();
        fields.init();
        childClasses.init();
    }

    AccessModifier getAccessModifier() { return modifier; }
    long getSerial() { return serial; }
    string getName() { return name; }
    string getModuleName() { return module_name; }
    ClassObject* getSuperClass() { return super; }
    ClassObject* getBaseClass() { return base; }
    ClassObject* getHeadClass() { return head; }
    bool match(ClassObject* klass) {
        return klass != NULL && klass->serial == serial;
    }
    void setBaseClass(ClassObject* base) {
        this->base = base;
    }
    void setSuperClass(ClassObject* sup) {
        this->super = sup;
    }
    void setHead(ClassObject* sup) {
        this->head = sup;
    }
    void setFullName(const string fullName) {
        this->fullName = fullName;
    }
    string getFullName() {
        return fullName;
    }

    void operator=(ClassObject& klass) {
        this->base = klass.base;
        this->childClasses = klass.childClasses;
        this->constructors = klass.constructors;
        this->fields = klass.fields;
        this->fullName = klass.fullName;
        this->functions = klass.functions;
        this->head = klass.head;
        this->modifier = klass.modifier;
        this->module_name = klass.module_name;
        this->name = klass.name;
        this->note = klass.note;
        this->overloads = klass.overloads;
        this->super = klass.super;
        this->serial = klass.serial;
        this->address = klass.address;
    }

    size_t constructorCount();
    Method* getConstructor(int p);
    Method* getConstructor(List<Param>& params, bool useBase =false);
    bool addConstructor(Method constr);

    size_t functionCount();
    Method* getFunction(int p);
    Method* getFunction(string name, List<Param>& params, bool useBase =false);
    Method* getFunction(string name, int64_t _offset);
    bool addFunction(Method function);

    size_t overloadCount();
    OperatorOverload* getOverload(size_t p);
    OperatorOverload* getPostIncOverload();
    OperatorOverload* getPostDecOverload();
    OperatorOverload* getPreIncOverload();
    OperatorOverload* getPreDecOverload();
    OperatorOverload* getOverload(Operator op, List<Param>& params, bool useBase =false);
    OperatorOverload* getOverload(Operator op, int64_t _offset);
    bool hasOverload(Operator op);
    bool addOperatorOverload(OperatorOverload overload);

    size_t fieldCount();
    Field* getField(int p);
    Field* getField(string name, bool ubase =false);
    bool addField(Field field);

    size_t childClassCount();
    ClassObject* getChildClass(int p);
    ClassObject* getChildClass(string name);
    bool addChildClass(ClassObject constr);
    void free();

    RuntimeNote note;

    bool isCurcular(ClassObject *pObject);

    bool matchBase(ClassObject *pObject);

    bool hasBaseClass(ClassObject *pObject);

    long getFieldIndex(string name);

    int baseClassDepth(ClassObject *pObject);

    long getTotalFieldCount();

    long getTotalFunctionCount();
    long getFieldAddress(Field* field);

    long long address;
private:
    AccessModifier modifier;
    long serial;
    string name;
    string fullName;
    string module_name;
    List<Method> constructors;
    List<Method> functions;
    List<OperatorOverload> overloads;
    List<Field> fields;
    List<ClassObject> childClasses;
    ClassObject *super, *base, *head;
};

#define totalFucntionCount(x) x->functionCount()+x->constructorCount()+x->overloadCount()


#endif //SHARP_CLASSOBJECT_H
