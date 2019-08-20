//
// Created by BraxtonN on 2/2/2018.
//

#include "Field.h"
#include "ClassObject.h"
#include "Runtime.h"

bool Field::operator==(Field& f)
{
    if(f.type == this->type && f.isArray == this->isArray) {
        if(type == CLASS)
            return f.nullType || (klass != NULL && klass->match(f.klass));
        else {
            if(prototype) {
                Expression expr = RuntimeEngine::fieldToExpression(NULL, f);
                return RuntimeEngine::prototypeEquals(this, expr.utype.getParams(), expr.utype.getReturnType());
            }
            return true;
        }
    } else if(f.nullType && !isArray){
        return type == CLASS || type==OBJECT;
    } else if(f.nullType && isArray){
        return type == CLASS || type==OBJECT || type == VAR;
    } else if(type == OBJECT) {
        if(f.type == OBJECT) {
            return isArray==f.isArray;
        } else if(f.type == VAR) {
            return !isArray==f.isArray;
        }
        return f.type == CLASS;
    }
    return false;
}

List<Param>& Field::getParams() {
    return proto==NULL ? params : proto->getParams();
}

void Field::free() {
    klass = NULL;
    owner = NULL;

    name.clear();
    fullName.clear();
    modifiers.free();
    key.clear();
    params.free();
    if(defValExpr != NULL) {
        defValExpr->free();
        delete defValExpr;
    }
}

void Field::operator=(Field f)
{
    free();

    type = f.type;
    klass = f.klass;
    serial = f.serial;
    name = f.name;
    fullName = f.fullName;
    owner = f.owner;
    modifiers.addAll(f.modifiers);
    isArray = f.isArray;
    nullType = f.nullType;
    address=f.address;
    local=f.local;
    key=f.key;
    ast=f.ast;
    proto=f.proto;
    prototype=f.prototype;
    returnType=f.returnType;
    params.addAll(f.params);
    isEnum=f.isEnum;
    constant_value=f.constant_value;
    locality=f.locality;
    thread_address=f.thread_address;
    defaultValue = f.defaultValue;
    if(f.defaultValue) {
        defValExpr = new Expression();
        *defValExpr = *f.defValExpr;
    } else
        defValExpr = NULL;
}
