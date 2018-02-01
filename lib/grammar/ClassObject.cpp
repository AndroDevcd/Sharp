//
// Created by BraxtonN on 2/1/2018.
//

#include "ClassObject.h"
#include "Param.h"
#include "runtime.h"

size_t ClassObject::constructorCount() {
    return constructors.size();
}

bool ClassObject::addChildClass(ClassObject klass) {
    if(getChildClass(klass.name) != NULL) {
        if(this->getChildClass(klass.name) || this->name == klass.name)
            return false;
        return false;
    }

    childClasses.push_back(klass);
    return true;
}

Method* ClassObject::getConstructor(int p) {
    return &constructors.get(p);
}

Method *ClassObject::getConstructor(List<Param>& params) {
    for(unsigned long i = 0; i < constructors.size(); i++) {
        if(Param::match(constructors.get(i).getParams(), params))
            return &constructors.get(i);
    }

    return NULL;
}

bool ClassObject::addConstructor(Method constr) {
    if(getConstructor(*constr.getParams()) != NULL)
        return false;

    constructors.push_back(constr);
    return true;
}

size_t ClassObject::functionCount() {
    return functions.size();
}

Method* ClassObject::getFunction(int p) {
    return &functions.get(p);
}

Method *ClassObject::getFunction(string name, List<Param>& params) {
    for(unsigned long i = 0; i < functions.size(); i++) {
        if(Param::match(functions.get(i).getParams(), params) && name == functions.get(i).getName())
            return &functions.get(i);
    }

    return NULL;
}

bool ClassObject::addFunction(Method function) {
    if(getFunction(function.getName(), *function.getParams()) != NULL)
        return false;

    functions.push_back(function);
    return true;
}

size_t ClassObject::fieldCount() {
    return fields.size();
}

Field* ClassObject::getField(int p) {
    return &fields.get(p);
}

Field* ClassObject::getField(string name) {
    for(unsigned long i = 0; i < fields.size(); i++) {
        if(fields.get(i).name == name)
            return &fields.get(i);
    }

    return NULL;
}

bool ClassObject::addField(Field field) {
    if(getField(field.name) != NULL)
        return false;

    fields.push_back(field);
    return true;
}

size_t ClassObject::childClassCount() {
    return childClasses.size();
}

ClassObject* ClassObject::getChildClass(int p) {
    return &childClasses.get(p);
}

ClassObject* ClassObject::getChildClass(string name) {
    for(unsigned long i = 0; i < childClasses.size(); i++) {
        if(childClasses.get(i).name == name)
            return &childClasses.get(i);
    }

    return NULL;
}

void ClassObject::free() {

}

size_t ClassObject::overloadCount() {
    return overloads.size();
}

OperatorOverload *ClassObject::getOverload(size_t p) {
    return &overloads.get(p);
}

OperatorOverload *ClassObject::getOverload(_operator op, List<Param> &params) {
    for(unsigned long i = 0; i < overloads.size(); i++) {
        if(Param::match(*overloads.get(i).getParams(), params) && op == overloads.get(i).getOperator())
            return &overloads.get(i);
    }

    return NULL;
}

bool ClassObject::addOperatorOverload(OperatorOverload overload) {
    if(getOverload(overload.getOperator(), *overload.getParams()) != NULL)
        return false;

    overloads.push_back(overload);
    return true;
}

int cSuper = 1;
bool ClassObject::isCurcular(ClassObject *pObject) {
    cSuper--;

    if(pObject == NULL)
        return false;
    if(cSuper == 0 && super != NULL) {
        cSuper++;
        return super->isCurcular(pObject);
    }

    for(unsigned long i = 0; i < childClasses.size(); i++) {
        if(childClasses.get(i).match(pObject) || childClasses.get(i).isCurcular(pObject)) {
            cSuper = 0;
            return true;
        }
    }

    cSuper = 0;
    return false;
}

bool ClassObject::matchBase(ClassObject *pObject) {
    return base != NULL && pObject != NULL && pObject->base != NULL
           && base->match(pObject->base);
}