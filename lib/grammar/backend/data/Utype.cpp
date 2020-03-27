//
// Created by braxtonn on 11/11/2019.
//

#include "Utype.h"
#include "../oo/Method.h"
#include "../oo/ClassObject.h"
#include "Literal.h"

string stackInjector = "stack-injector";
string ebxInjector = "ebx-injector";
string ptrInjector = "ptr-injector";

void Utype::free() {

    if(type == utype_native && resolvedType) {
        resolvedType->release();
        delete resolvedType;
    }
    if(type == utype_literal && resolvedType) {
        ((Literal*)resolvedType)->free();
        delete resolvedType;
    }
    code.free();
}

void Utype::softFree() {
    code.free();
}

string Utype::toString() {
    stringstream ss;
    if(resolvedType == NULL) {
        ss << "?";
    } else {
        if (type == utype_native) {
            switch (resolvedType->type) {
                case _INT8:
                    ss << "_int8";
                    break;
                case _INT16:
                    ss << "_int16";
                    break;
                case _INT32:
                    ss << "_int32";
                    break;
                case _INT64:
                    ss << "_int64";
                    break;
                case _UINT8:
                    ss << "_uint8";
                    break;
                case _UINT16:
                    ss << "_uint16";
                    break;
                case _UINT32:
                    ss << "_uint32";
                    break;
                case _UINT64:
                    ss << "_uint64";
                    break;
                case VAR:
                    ss << "var";
                    break;
                case OBJECT:
                    ss << "object";
                    break;
                case NIL:
                    ss << "nil";
                    break;
                case FNPTR:
                    ss << ((Method *) resolvedType)->toString();
                    break;
                default:
                    ss << "<undefined>";
                    break;
            }

            if(array) ss << "[]";
        } else if (type == utype_class) {
            ss << resolvedType->fullName;
            if(array) ss << "[]";
        } else if (type == utype_field) {
            ss << ((Field *) resolvedType)->toString();
        } else if (type == utype_method || type == utype_function_ptr) {
            ss << ((Method *) resolvedType)->toString();
        } else if (type == utype_literal) {
            if (((Literal *) resolvedType)->literalType == numeric_literal)
                ss << "[literal: " << ((Literal *) resolvedType)->numericData << "]";
            else
                ss << "[literal: " << ((Literal *) resolvedType)->stringData << "]";
        } else {
            ss << "<undefined>";
        }
    }

    return ss.str();
}

Utype::Utype(ClassObject *k, bool isArray)
    :
            type(utype_class),
            array(isArray),
            resolvedType(k)
    {
        code.init();
    }

Utype::Utype(DataType type, bool isArray)
    :
        type(utype_native),
        array(isArray),
        resolvedType(new DataEntity())
    {
        resolvedType->type = type;
        code.init();
    }

bool Utype::equals(Utype *utype) {
    if(type == utype_method || type == utype_function_ptr) {
        if(Compiler::simpleParameterMatch(((Method*)resolvedType)->params, ((Method*)utype->resolvedType)->params)) {
            if((((Method*)resolvedType)->utype != NULL && ((Method*)utype->resolvedType)->utype != NULL) ||
               (((Method*)resolvedType)->utype == NULL && ((Method*)utype->resolvedType)->utype == NULL)) {
                if(((Method*)resolvedType)->utype != NULL) {
                    return ((Method*)resolvedType)->utype->equals(((Method*)utype->resolvedType)->utype);
                } else
                    return true;
            }
        }
    } else if(type == utype->type) {
        if(type != utype_unresolved) {
            if(type == utype_native)
                return resolvedType->type == utype->resolvedType->type && array == utype->array;
            else if(type == utype_class)
                return ((ClassObject*)resolvedType)->match((ClassObject*)utype->resolvedType) && array == utype->array;
            else if(type == utype_field)
                return ((Field*)resolvedType)->equals(*(Field*)utype->resolvedType);
        } else
            return true;
    }
    return false;
}

bool Utype::isRelated(Utype *utype) {

    if(resolvedType && utype->getResolvedType()) {
        if (getClass()) {
            if (utype->getClass())
                return getClass()->isClassRelated(utype->getClass()) && array == utype->array;
        } else if (resolvedType->type == OBJECT) {
            if(array) {
                if (utype->array && (utype->getClass() || utype->getResolvedType()->type == OBJECT))
                    return true;
            } else {
                if(utype->resolvedType->isVar())
                    return utype->array;
                else
                    return true;
            }
        } else if (resolvedType->type == VAR || (resolvedType->type >= _INT8 && resolvedType->type <= _UINT64)) {
            if (utype->getResolvedType()->type >= _INT8 && utype->getResolvedType()->type <= VAR)
                return array == utype->array;
            else if(array && utype->nullType)
                return true;
        } else if (resolvedType->type == FNPTR) {
            if (utype->getResolvedType()->type == FNPTR || utype->getType() == utype_method) {
                return Compiler::simpleParameterMatch(((Method *) resolvedType)->params,
                                                      ((Method *) utype->getResolvedType())->params);
            }
        } else if(type == utype_method) {
            if (utype->getResolvedType()->type >= _INT8 && utype->getResolvedType()->type <= VAR)
                return true;
        }
    }
    return false;
}

bool Utype::isClass()  {
    return type == utype_class || (type == utype_field && ((Field*)resolvedType)->utype->isClass());
}

ClassObject *Utype::getClass() {
    if(isClass()) {
        if(type == utype_class)
            return (ClassObject*)resolvedType;
        else return ((Field*)resolvedType)->utype->getClass();
    }

    return NULL;
}

