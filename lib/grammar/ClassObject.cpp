//
// Created by BraxtonN on 1/31/2017.
//

#include "ClassObject.h"
#include "Param.h"
#include "runtime.h"

size_t ClassObject::constructorCount() {
    return constructors->size();
}

bool ClassObject::addChildClass(ClassObject klass) {
    if(getChildClass(klass.name) != NULL) {
        if(this->getChildClass(klass.name) || this->name == klass.name)
            return false;
        return false;
    }

    klass.parent = this;
    childClasses->push_back(klass);
    return true;
}

Method* ClassObject::getConstructor(int p) {
    return &(*std::next(constructors->begin(), p));
}

Method *ClassObject::getConstructor(list<Param>& params) {
    for(Method& function : *constructors) {
        if(Param::match(*function.getParams(), params))
            return &function;
    }

    return NULL;
}

bool ClassObject::addConstructor(Method constr) {
    if(getConstructor(*constr.getParams()) != NULL)
        return false;

    constructors->push_back(constr);
    return true;
}

size_t ClassObject::functionCount() {
    return functions->size();
}

Method* ClassObject::getFunction(int p) {
    return &(*std::next(functions->begin(), p));
}

Method *ClassObject::getFunction(string name, list<Param>& params) {
    for(Method& function : *functions) {
        if(Param::match(*function.getParams(), params) && name == function.getName())
            return &function;
    }

    return NULL;
}

bool ClassObject::addFunction(Method function) {
    if(getFunction(function.getName(), *function.getParams()) != NULL)
        return false;

    functions->push_back(function);
    return true;
}

size_t ClassObject::fieldCount() {
    return fields->size();
}

Field* ClassObject::getField(int p) {
    return &(*std::next(fields->begin(), p));
}

Field* ClassObject::getField(string name) {
    for(Field& field : *fields) {
        if(field.name == name)
            return &field;
    }

    return NULL;
}

bool ClassObject::addField(Field field) {
    if(getField(field.name) != NULL)
        return false;

    fields->push_back(field);
    return true;
}

size_t ClassObject::childClassCount() {
    return childClasses->size();
}

ClassObject* ClassObject::getChildClass(int p) {
    return &(*std::next(childClasses->begin(), p));
}

ClassObject* ClassObject::getChildClass(string name) {
    for(ClassObject& klass : *childClasses) {
        if(klass.name == name)
            return &klass;
    }

    return NULL;
}

void ClassObject::free() {

}

size_t ClassObject::overloadCount() {
    return overloads->size();
}

OperatorOverload *ClassObject::getOverload(size_t p) {
    return &element_at(*overloads, p);
}

OperatorOverload *ClassObject::getOverload(_operator op, list<Param> &params) {
    for(OperatorOverload& oper : *overloads) {
        if(Param::match(*oper.getParams(), params) && op == oper.getOperator())
            return &oper;
    }

    return NULL;
}

bool ClassObject::addOperatorOverload(OperatorOverload overload) {
    if(getOverload(overload.getOperator(), *overload.getParams()) != NULL)
        return false;

    overloads->push_back(overload);
    return true;
}

size_t ClassObject::macrosCount() {
    return macros->size();
}

Method *ClassObject::getMacros(int p) {
    return &element_at(*macros,p);
}

Method *ClassObject::getMacros(string name, list<Param> &params) {
    for(Method& macro : *macros) {
        if(Param::match(*macro.getParams(), params) && name == macro.getName())
            return &macro;
    }

    return NULL;
}

bool ClassObject::addMacros(Method macro) {
    if(getFunction(macro.getName(), *macro.getParams()) != NULL)
        return false;

    macros->push_back(macro);
    return true;
}
