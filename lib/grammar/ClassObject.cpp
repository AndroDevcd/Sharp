//
// Created by BraxtonN on 1/31/2017.
//

#include "ClassObject.h"
#include "Param.h"

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
    if(getConstructor(*constr.getParams()))
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
    if(getFunction(function.getName(), *function.getParams()))
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
