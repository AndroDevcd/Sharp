//
// Created by BraxtonN on 10/22/2019.
//

#include "Field.h"
#include "../Compiler.h"
#include "ClassObject.h"
#include "Method.h"
#include "../Expression.h"

void Field::free() {
    release();
}

bool Field::equals(Field &f) {
    if(f.type == ANY)
        return true;

    if(f.type == this->type && f.isArray == this->isArray) {
        if(type == CLASS)
            return (utype != NULL && f.utype != NULL && utype->getType() == f.utype->getType() &&
                    ((ClassObject*)utype->getResolvedType())->match((ClassObject*)f.utype->getResolvedType()));
        else {
            if(type==FNPTR) {
                Method *compareFun = f.utype->getMethod();
                if(compareFun->fnType == fn_lambda) {
                    if(compareFun->utype != NULL)
                        return utype != NULL && f.utype != NULL && utype->getType() == f.utype->getType() &&
                               Compiler::simpleParameterMatch(((Method*)this->utype->getResolvedType())->params,
                                                              compareFun->params);
                    else
                        return Compiler::simpleParameterMatch(((Method*)this->utype->getResolvedType())->params,
                                                              compareFun->params);
                } else {
                    return utype != NULL && f.utype != NULL && utype->getType() == f.utype->getType() &&
                           Compiler::simpleParameterMatch(((Method *) this->utype->getResolvedType())->params,
                                                          compareFun->params);
                }
            }
            return true;
        }
    }
    return false;
}

string Field::toString() {
    stringstream ss;
    if(fullName != "")
        ss << fullName << ": ";

    if(type == UNTYPED)
        ss << "<untyped>";
    else if(type == ANY)
        ss << "Any?";
    else if(utype)
        ss << utype->toString();
    else
        ss << "?";
    return ss.str();
}

bool Field::isRelated(Field &f) {
    if(type == CLASS) {
        if(f.type == CLASS)
            return isArray==f.isArray && ((ClassObject*)f.utype->getResolvedType())->isClassRelated((ClassObject*)utype->getResolvedType());
    } else if(type == OBJECT) {
        if(f.type == CLASS || f.type == OBJECT
            || (f.isVar() && f.isArray && !isArray))
            return true;
    }  else if(type == VAR) {
        if(isArray)
            return f.isArray && f.type == VAR;
        else {
            if (f.type <= VAR)
                return isArray == f.isArray;
        }
    } else if(type == FNPTR) {
        if(f.type == FNPTR) {
            Method *compareFun = (Method*)f.utype->getResolvedType();
            if(compareFun->fnType == fn_lambda) {
                if(compareFun->utype != NULL)
                    return isArray==f.isArray && utype != NULL && f.utype != NULL && utype->getType() == f.utype->getType() &&
                           Compiler::simpleParameterMatch(((Method*)this->utype->getResolvedType())->params,
                                                          compareFun->params);
                else
                    return isArray==f.isArray && Compiler::simpleParameterMatch(((Method*)this->utype->getResolvedType())->params,
                                                          compareFun->params);
            } else {
                return isArray==f.isArray && utype != NULL && f.utype != NULL && utype->getType() == f.utype->getType() &&
                       Compiler::simpleParameterMatch(((Method *) this->utype->getResolvedType())->params,
                                                      compareFun->params);
            }
        }
    } else if(type >= _INT8 && type <= _UINT64) {
        if(isArray)
            return f.isArray && f.type == VAR;
        else {
            if (f.type <= VAR)
                return isArray == f.isArray;
        }
    }
    return false;
}
