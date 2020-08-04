//
// Created by braxtonn on 11/11/2019.
//

#include "Utype.h"
#include "../oo/Method.h"
#include "../oo/ClassObject.h"
#include "Literal.h"

native_string stackInjector("stack-injector");
native_string ebxInjector("ebx-injector");
native_string ptrInjector("ptr-injector");
native_string getterInjector("getterCodeSize-injector");
native_string removeFromStackInjector("removeFromStack-injector");
native_string incInjector("inc-injector");
native_string decInjector("dec-injector");
native_string indexAssignInjector("indexAssign-injector");

void Utype::free() {
    code.free();
    type = utype_unresolved;
    resolvedType = NULL;
    array = false;
    nullType = false;
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
            if(array && !((Field *) resolvedType)->isArray) ss << "[]";
        } else if (type == utype_method || type == utype_function_ptr) {
            ss << ((Method *) resolvedType)->toString();
            if(array) ss << "[]";
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
            resolvedType(k),
            nullType(false)
    {
        code.init();
    }

Utype::Utype(DataType type, bool isArray)
    :
        type(utype_native),
        array(isArray),
        nullType(false),
        resolvedType(new DataEntity())
    {
        resolvedType->type = type;
        code.init();
    }

bool Utype::equals(Utype *utype) {
    if(type == utype_method || type == utype_function_ptr) {
        if(utype->getMethod() != NULL && Compiler::simpleParameterMatch(getMethod()->params, utype->getMethod()->params)) {
            if((getMethod()->utype != NULL && utype->getMethod()->utype != NULL) ||
               (getMethod()->utype == NULL && utype->getMethod()->utype == NULL)) {
                if(getMethod()->utype != NULL) {
                    return getMethod()->utype->equals(utype->getMethod()->utype);
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

    if(resolvedType && utype && utype->getResolvedType()) {
        if (getClass()) {
            if (utype->getClass())
                return getClass()->isClassRelated(utype->getClass()) && array == utype->array;
            else return utype->nullType;
        } else if (resolvedType->type == OBJECT) {
            if(array) {
                if (utype->array && (utype->getClass() || utype->getResolvedType()->type == OBJECT))
                    return true;
                else return utype->nullType;
            } else {
                if(utype->resolvedType->isVar())
                    return utype->array;
                else
                    return utype->getResolvedType()->type <= CLASS;
            }
        } else if (resolvedType->type == VAR || (resolvedType->type >= _INT8 && resolvedType->type <= _UINT64)) {
            if (utype->getResolvedType()->type <= VAR)
                return isArray() == utype->isArray();
            else if(array && utype->nullType)
                return true;
        } else if (type == utype_method || resolvedType->type == FNPTR) {
            if (utype->getResolvedType()->type == FNPTR || utype->getType() == utype_method) {
                return Compiler::simpleParameterMatch(getMethod()->params,
                                                      utype->getMethod()->params);
            } else if(utype->nullType && array)
                return true;
        } else if(resolvedType->type == NIL) {
            return utype->getResolvedType()->type == NIL;
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

Method *Utype::getMethod() {
    if(type == utype_method || type == utype_function_ptr)
        return (Method*)resolvedType;
    else if(type == utype_field) return ((Field*)resolvedType)->utype->getMethod();
    else return NULL;
}

bool Utype::isArray() {
    return type == utype_field ? ((Field*)resolvedType )->isArray : array;
}

