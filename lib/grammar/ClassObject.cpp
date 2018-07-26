//
// Created by BraxtonN on 2/1/2018.
//

#include "ClassObject.h"
#include "Param.h"
#include "Runtime.h"

size_t ClassObject::constructorCount() {
    return constructors.size();
}

bool ClassObject::addChildClass(ClassObject *klass) {
    if(getChildClass(klass->name) != NULL) {
        if(this->getChildClass(klass->name) || this->name == klass->name) {
            delete klass;
            return false;
        }
        delete klass;
        return false;
    }

    //klass.address = RuntimeEngine::uniqueSerialId++;
    childClasses.push_back(klass);
    return true;
}

Method* ClassObject::getConstructor(int p) {
    return &constructors.get(p);
}

Method *ClassObject::getConstructor(List<Param>& params, bool useBase, bool nativeSupport, bool ambiguousProtect, bool find) {
    for(unsigned long i = 0; i < constructors.size(); i++) {
        if(Param::match(constructors.get(i).getParams(), params, nativeSupport, ambiguousProtect, find))
            return &constructors.get(i);
    }

    if(useBase && base != NULL)
        return base->getConstructor(params, useBase);

    return NULL;
}

bool ClassObject::addConstructor(Method constr) {
    if(getConstructor(constr.getParams(), false, false, true, false) != NULL)
        return false;

    constructors.push_back(constr);
    return true;
}

size_t ClassObject::functionCount(bool ignore) {
    size_t count = 0;
    for(unsigned int i = 0; i < functions.size(); i++) {
        Method& function = functions.get(i);
        if(ignore || !function.delegatePost) {
            count++;
        }
    }
    return count;
}

Method* ClassObject::getFunction(int p) {
    return &functions.get(p);
}

Method *ClassObject::getFunction(string name, List<Param>& params, bool useBase, bool nativeSupport, bool skipdelegates, bool ambiguousProtect, bool find) {
    for(unsigned long i = 0; i < functions.size(); i++) {
        if(Param::match(functions.get(i).getParams(), params, nativeSupport, ambiguousProtect, find) && name == functions.get(i).getName()) {
            if(skipdelegates && !functions.get(i).delegate)
                return &functions.get(i);
            else
                return &functions.get(i);

        }
    }

    if(useBase && base != NULL)
        return base->getFunction(name, params, useBase);

    return NULL;
}

Method *ClassObject::getDelegatePost(string name, List<Param>& params, bool useBase, bool nativeSupport, bool find) {
    for(unsigned long i = 0; i < functions.size(); i++) {
        if(Param::match(functions.get(i).getParams(), params, nativeSupport, true, find) && name == functions.get(i).getName()) {
            if(functions.get(i).delegatePost)
                return &functions.get(i);

        }
    }

    if(useBase && base != NULL)
        return base->getDelegatePost(name, params, useBase, nativeSupport);

    return NULL;
}

Method *ClassObject::getDelegateFunction(string name, List<Param>& params, bool useBase, bool nativeSupport) {
    Method* fn = NULL;
    if((fn = getDelegatePost(name, params, useBase, nativeSupport)) != NULL) {
        return fn;
    }

    ClassObject* iface;
    for(unsigned long i = 0; i < interfaces.size(); i++) {
        iface = interfaces.get(i);
        if((fn = iface->getFunction(name, params, true, true, false)) != NULL) {
            return fn;
        }
    }

    if(useBase && base != NULL)
        return base->getDelegateFunction(name, params, useBase, nativeSupport);

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
    if(getFunction(function.getName(), function.getParams(), false, false, false, true, false) != NULL)
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
    return childClasses.get(p);
}

ClassObject* ClassObject::getChildClass(string name) {
    for(unsigned long i = 0; i < childClasses.size(); i++) {
        if(childClasses.get(i)->name == name)
            return childClasses.get(i);
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
    RuntimeEngine::freeListPtr(childClasses);
    RuntimeEngine::freeListPtr(interfaces);
}

size_t ClassObject::overloadCount() {
    return overloads.size();
}

OperatorOverload *ClassObject::getOverload(size_t p) {
    return &overloads.get(p);
}

OperatorOverload *ClassObject::getOverload(Operator op, List<Param> &params, bool useBase, bool nativeSupport, bool ambiguousProtect, bool find) {
    for(unsigned long i = 0; i < overloads.size(); i++) {
        if(Param::match(overloads.get(i).getParams(), params, nativeSupport, ambiguousProtect, find) && op == overloads.get(i).getOperator())
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
    if(getOverload(overload.getOperator(), overload.getParams(), false, false, true, false) != NULL)
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
        if(childClasses.get(i)->match(pObject) || childClasses.get(i)->isCurcular(pObject)) {
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
    if(base == NULL) return false;
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
            if(oper.getParams().size() == 1 && oper.getParams().last().field.isVar() && !oper.getParams().last().field.isArray) {
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
            if(oper.getParams().size() == 1 && oper.getParams().last().field.isVar() && !oper.getParams().last().field.isArray) {
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

List<Method *> ClassObject::getDelegatePosts(bool ubase) {
    List<Method*> delegatePosts, tmp;
    ClassObject* k, *_klass = this;

    for(int i = 0; i < functions.size(); i++) {
        Method *func = getFunction(i);
        if(func->delegatePost)
            delegatePosts.add(func);
    }

    if(ubase) {
        for(;;) {
            k = _klass->getBaseClass();

            if(k == NULL)
                return delegatePosts;

            _klass = k;
            tmp = k->getDelegatePosts(ubase);
            delegatePosts.appendAll(tmp);
        }
    }

    return delegatePosts;
}

List<Method *> ClassObject::getDelegates() {
    List<Method*> delegates;

    for(int i = 0; i < functions.size(); i++) {
        Method *func = getFunction(i);
        if(func->delegate)
            delegates.add(func);
    }

    return delegates;
}

bool ClassObject::addGenericType(Expression *utype) {
    genericMap.push_back(*utype);
    return false;
}

Expression *ClassObject::getGenericType(string &key) {
    if(genericKeys.find(key)) {
        return &genericMap.get(genericKeys.indexof(key));
    }

    return nullptr;
}

Method *ClassObject::getFunctionByName(string name, bool &ambiguous) {
    Method *func = NULL;
    for(size_t i = 0; i < functions.size(); i++) {
        if(name == functions.get(i).getName()) {
            if(func == NULL)
                func = &functions.get(i);
            else
                ambiguous = true;
        }
    }
    return func;
}
