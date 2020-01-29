//
// Created by braxtonn on 11/11/2019.
//

#include "Utype.h"
#include "oo/Method.h"
#include "oo/ClassObject.h"
#include "Literal.h"

string stackInjector = "stack-injector";
string fnPtrInjector = "fnPtr-injector";

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
                case _VOID:
                    ss << "void";
                    break;
                case FNPTR:
                    ss << ((Method *) resolvedType)->toString();
                    break;
                default:
                    ss << "<error>";
                    break;
            }
        } else if (type == utype_class) {
            ss << resolvedType->fullName;
        } else if (type == utype_field) {
            ss << ((Field *) resolvedType)->toString();
        } else if (type == utype_method || type == utype_method_prototype) {
            ss << ((Method *) resolvedType)->toString();
        } else if (type == utype_literal) {
            if (((Literal *) resolvedType)->literalType == numeric_literal)
                ss << "[literal: " << ((Literal *) resolvedType)->numericData << "]";
            else
                ss << "[literal: " << ((Literal *) resolvedType)->stringData << "]";
        } else {
            ss << "unresolved";
        }
    }

    if(array) ss << "[]";
    return ss.str();
}

Utype::Utype(ClassObject *k, bool isArray)
    :
            type(utype_class),
            array(isArray),
            resolvedType(k)
    {
        code.init();
        code.addinjector(stackInjector);
    }

Utype::Utype(DataType type, bool isArray)
    :
        type(utype_native),
        array(isArray),
        resolvedType(new DataEntity())
    {
        resolvedType->type = type;
        code.init();
        code.addinjector(stackInjector);
    }

bool Utype::equals(Utype *utype) {
    if(type == utype->type) {
        if(type != utype_unresolved) {
            if(type == utype_native)
                return resolvedType->type == utype->resolvedType->type;
            else if(type == utype_class)
                return ((ClassObject*)resolvedType)->match((ClassObject*)utype->resolvedType);
            else if(type == utype_method || type == utype_method_prototype) {
                if(Compiler::simpleParameterMatch(((Method*)resolvedType)->params, ((Method*)utype->resolvedType)->params)) {
                    if((((Method*)resolvedType)->utype != NULL && ((Method*)utype->resolvedType)->utype != NULL) ||
                       (((Method*)resolvedType)->utype == NULL && ((Method*)utype->resolvedType)->utype == NULL)) {
                        if(((Method*)resolvedType)->utype != NULL) {
                            return ((Method*)resolvedType)->utype->equals(((Method*)utype->resolvedType)->utype);
                        } else
                            return true;
                    }
                }
            }
            else if(type == utype_field)
                return ((Field*)resolvedType)->equals(*(Field*)utype->resolvedType);
        } else
            return true;
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

