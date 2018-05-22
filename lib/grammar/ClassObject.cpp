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

    //klass.address = RuntimeEngine::uniqueSerialId++;
    childClasses.push_back(klass);
    return true;
}

Method* ClassObject::getConstructor(int p) {
    return &constructors.get(p);
}

Method *ClassObject::getConstructor(List<Param>& params, bool useBase) {
    for(unsigned long i = 0; i < constructors.size(); i++) {
        if(Param::match(constructors.get(i).getParams(), params))
            return &constructors.get(i);
    }

    if(useBase && base != NULL)
        return base->getConstructor(params, useBase);

    return NULL;
}

bool ClassObject::addConstructor(Method constr) {
    if(getConstructor(constr.getParams()) != NULL)
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

Method *ClassObject::getFunction(string name, List<Param>& params, bool useBase) {
    for(unsigned long i = 0; i < functions.size(); i++) {
        if(Param::match(functions.get(i).getParams(), params) && name == functions.get(i).getName())
            return &functions.get(i);
    }

    if(useBase && base != NULL)
        return base->getFunction(name, params, useBase);

    return NULL;
}

Method *ClassObject::getFunction(string name, int64_t _offset) {
    for(unsigned int i = 0; i < functions.size(); i++) {
        Method& function = functions.get(i);
        if(name == function.getName()) {
            if(_offset == 0)
                return &function;
            else
                _offset--;
        }
    }

    return NULL;
}

bool ClassObject::addFunction(Method function) {
    if(getFunction(function.getName(), function.getParams()) != NULL)
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

Field* ClassObject::getField(string name, bool useBase) {
    for(unsigned long i = 0; i < fields.size(); i++) {
        if(fields.get(i).name == name)
            return &fields.get(i);
    }

    if(useBase && base != NULL)
        return base->getField(name, useBase);

    return NULL;
}

long ClassObject::getFieldIndex(string name) {
    long iter = 0;
    for(unsigned int i = 0; i < fields.size(); i++) {
        Field& field = fields.get(i);
        if(field.name == name)
            return iter;
        iter++;
    }

    return iter;
}

bool ClassObject::addField(Field field) {
    if(getField(field.name) != NULL)
        return false;

    stringstream ss;
    ss << this->fullName << "." << field.name;
    field.fullName = ss .str();
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
    name.clear();
    fullName.clear();
    module_name.clear();
    RuntimeEngine::freeList(constructors);
    RuntimeEngine::freeList(functions);
    RuntimeEngine::freeList(overloads);
    RuntimeEngine::freeList(fields);
    RuntimeEngine::freeList(childClasses);
}

size_t ClassObject::overloadCount() {
    return overloads.size();
}

OperatorOverload *ClassObject::getOverload(size_t p) {
    return &overloads.get(p);
}

OperatorOverload *ClassObject::getOverload(Operator op, List<Param> &params, bool useBase) {
    for(unsigned long i = 0; i < overloads.size(); i++) {
        if(Param::match(overloads.get(i).getParams(), params) && op == overloads.get(i).getOperator())
            return &overloads.get(i);
    }


    if(useBase && base != NULL)
        return base->getOverload(op, params, useBase);

    return NULL;
}

OperatorOverload *ClassObject::getOverload(Operator op, int64_t _offset) {
    for(unsigned int i = 0; i < overloads.size(); i++) {
        OperatorOverload& oper = overloads.get(i);
        if(op == oper.getOperator()) {
            if(_offset == 0)
                return &oper;
            else
                _offset--;
        }
    }

    return NULL;
}

bool ClassObject::addOperatorOverload(OperatorOverload overload) {
    if(getOverload(overload.getOperator(), overload.getParams()) != NULL)
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
bool ClassObject::hasBaseClass(ClassObject *pObject) {
    if(base == NULL) return true;
    ClassObject* k, *_klass = this;

    for(;;) {
        k = _klass->getBaseClass();

        if(k == NULL)
            return false;

        if(k->match(pObject)) {
            return true;
        }else
            _klass = k;
    }
}

int ClassObject::baseClassDepth(ClassObject *pObject) {
    if(base == NULL) return 0;
    ClassObject* k, *_klass = this;
    int depth=0;

    for(;;) {
        depth++;
        k = _klass->getBaseClass();

        if(k == NULL)
            return depth;

        if(k->match(pObject)) {
            return depth;
        }else
            _klass = k;
    }
}

bool ClassObject::hasOverload(Operator op) {
    for(unsigned int i = 0; i < overloads.size(); i++) {
        if(op == overloads.get(i).getOperator())
            return true;
    }

    return false;
}

OperatorOverload *ClassObject::getPostIncOverload() {
    for(unsigned int i = 0; i < overloads.size(); i++) {
        OperatorOverload& oper = overloads.get(i);
        if(oper_INC == oper.getOperator()) {
            if(oper.getParams().size() == 1 && oper.getParams().last().field.isVar()) {
                return &oper;
            }
        }
    }

    return NULL;
}

OperatorOverload *ClassObject::getPostDecOverload() {
    for(unsigned int i = 0; i < overloads.size(); i++) {
        OperatorOverload& oper = overloads.get(i);
        if(oper_DEC == oper.getOperator()) {
            if(oper.getParams().size() == 1 && oper.getParams().last().field.isVar()) {
                return &oper;
            }
        }
    }

    return NULL;
}

OperatorOverload *ClassObject::getPreIncOverload() {
    for(unsigned int i = 0; i < overloads.size(); i++) {
        OperatorOverload& oper = overloads.get(i);
        if(oper_INC == oper.getOperator()) {
            if(oper.getParams().size() == 0) {
                return &oper;
            }
        }
    }

    return NULL;
}

OperatorOverload *ClassObject::getPreDecOverload() {
    for(unsigned int i = 0; i < overloads.size(); i++) {
        OperatorOverload& oper = overloads.get(i);
        if(oper_DEC == oper.getOperator()) {
            if(oper.getParams().size() == 0) {
                return &oper;
            }
        }
    }

    return NULL;
}

long ClassObject::getTotalFieldCount() {
    if(base == NULL) return fieldCount();
    ClassObject* k, *_klass = this;
    long fields=fieldCount();

    for(;;) {
        k = _klass->getBaseClass();

        if(k == NULL)
            return fields;

        fields+=k->fieldCount();
        _klass = k;
    }
}

long ClassObject::getFieldAddress(Field* field) {
    if(base == NULL) return getFieldIndex(field->name);
    ClassObject* k, *_klass = this;
    long fields=0;

    for(;;) {
        k = _klass->getBaseClass();

        if(k == NULL)
            return fields+getFieldIndex(field->name);

        fields+=k->fieldCount();
        _klass = k;
    }
}

long ClassObject::getTotalFunctionCount() {
    if(base == NULL) return totalFucntionCount(this);
    ClassObject* k, *_klass = this;
    long total=totalFucntionCount(this);

    for(;;) {
        k = _klass->getBaseClass();

        if(k == NULL)
            return total;

        total+=totalFucntionCount(k);
        _klass = k;
    }
}
