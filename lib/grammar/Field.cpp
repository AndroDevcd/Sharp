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

List<Param> Field::getParams() {
    return proto==NULL ? params : proto->getParams();
}
