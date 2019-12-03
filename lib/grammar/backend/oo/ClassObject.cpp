//
// Created by BraxtonN on 10/22/2019.
//

#include "ClassObject.h"
#include "../Compiler.h"


void ClassObject::free()
{
    release();
    keys.free();
    Compiler::freeListPtr(fields);
    Compiler::freeListPtr(classes);
    Compiler::freeListPtr(keyTypes);
    Compiler::freeListPtr(functions);
}

Field* ClassObject::getField(string& name, bool checkBase) {
    Field *field = NULL;
    for(long i = 0; i < fields.size(); i++) {
        field = fields.get(i);
        if(field->name == name) {
            return field;
        }
    }

    if(checkBase && super)
        return super->getField(name, true);

    return NULL;
}

long ClassObject::getFieldIndex(string &name) {
    long iter = 0;
    for(unsigned int i = 0; i < fields.size(); i++) {
        if(fields.get(i)->name == name)
            return iter;
        iter++;
    }

    return iter;
}

long ClassObject::getFieldAddress(Field* field) {
    if(super == NULL) return getFieldIndex(field->name);
    ClassObject* k, *_klass = this;
    long fields=0;

    for(;;) {
        k = _klass->getSuperClass();

        if(k == NULL)
            return fields+getFieldIndex(field->name);

        fields+=k->fields.size();
        _klass = k;
    }
}

Method* ClassObject::getConstructor(List<Field*> params, bool checkBase) {
    Method *fun;
    List<Method*> constructors;
    getAllFunctionsByType(fn_constructor, constructors);

    for(long i = 0; i < constructors.size(); i++) {
        fun = constructors.get(i);
        if(Compiler::simpleParameterMatch(params, fun->params)) {
            constructors.free();
            return fun;
        }
    }

    if(checkBase) {
        constructors.free();
        return super->getConstructor(params, true);
    }

    constructors.free();
    return NULL;
}

bool ClassObject::isClassRelated(ClassObject *klass) {
    if(klass == NULL) return false;
    if(match(klass)) return true;

    for(long long i = 0; i < interfaces.size(); i++) {
        if(interfaces.get(i)->isClassRelated(klass))
            return true;
    }

    return super == NULL ? false : super->isClassRelated(klass);
}

bool ClassObject::getFunctionByName(string name, List<Method*> &funcs, bool checkBase) {
    bool found = false;
    for(size_t i = 0; i < functions.size(); i++) {
        if(name == functions.get(i)->name) {
            funcs.add(functions.get(i));
            found = true;
        }
    }
    if(checkBase && super)
        return super->getFunctionByName(name, funcs, true);
    return found;
}

