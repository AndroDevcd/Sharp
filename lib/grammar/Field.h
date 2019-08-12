//
// Created by BraxtonN on 2/2/2018.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../stdimports.h"
#include "FieldType.h"
#include "AccessModifier.h"
#include "RuntimeNote.h"
#include "List.h"

class ClassObject;
class Ast;
class Method;
class Param;

class Field {
public:
    Field(FieldType type, uint64_t serial, const string name, ClassObject* parent, List<AccessModifier>& modifiers,
          RuntimeNote& note, StorageLocality stl, int64_t taddr)
            :
            type(type),
            serial(serial),
            name(name),
            fullName(""),
            owner(parent),
            klass(NULL),
            note(note),
            isArray(false),
            nullType(false),
            local(false),
            key(""),
            ast(NULL),
            proto(NULL),
            isEnum(false),
            constant_value(0),
            prototype(false),
            returnType(TYPEVOID),
            locality(stl),
            thread_address(taddr)
    {
        this->modifiers.init();
        this->params.init();
        this->modifiers.addAll(modifiers);
    }

    Field(ClassObject* klass, uint64_t serial, const string name, ClassObject* parent, List<AccessModifier>& modifiers,
          RuntimeNote& note, StorageLocality stl, int64_t taddr)
            :
            type(UNDEFINED),
            serial(serial),
            name(name),
            fullName(""),
            owner(parent),
            klass(klass),
            note(note),
            isArray(false),
            nullType(false),
            local(false),
            key(""),
            ast(NULL),
            proto(NULL),
            isEnum(false),
            constant_value(0),
            prototype(false),
            returnType(TYPEVOID),
            locality(stl),
            thread_address(taddr)
    {
        this->modifiers.init();
        this->params.init();
        this->modifiers.addAll(modifiers);
    }

    Field()
            :
            type(UNDEFINED),
            serial(0),
            name(""),
            fullName(""),
            modifiers(),
            note("","",0,0),
            isArray(false),
            nullType(false),
            local(false),
            owner(NULL),
            key(""),
            klass(NULL),
            ast(NULL),
            proto(NULL),
            isEnum(false),
            constant_value(0),
            prototype(false),
            returnType(TYPEVOID),
            locality(stl_local),
            thread_address(0)
    {
    }

    bool operator==(Field& f);

    void operator=(Field f)
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
    }

    void free(){
        klass = NULL;
        owner = NULL;

        name.clear();
        fullName.clear();
        modifiers.free();
        key.clear();
        params.free();
    }

//    bool isField() {
//        return nf != fnof || nf >= fdynamic;
//    }

    bool isStatic() {
        for(unsigned int i = 0; i < modifiers.size(); i++) {
            if(modifiers.at(i) == STATIC)
                return true;
        }
        return false;
    }

    bool isConst() {
        for(unsigned int i = 0; i < modifiers.size(); i++) {
            if(modifiers.at(i) == mCONST)
                return true;
        }
        return false;
    }


    bool isVar() {
        return type == VAR;
    }

    bool isNative() {
        return isVar() || dynamicObject();
    }

    bool isObjectInMemory() {
        return type == CLASS || isArray || dynamicObject();
    }

    bool dynamicObject() {
        return type == OBJECT;
    }

    bool hasThreadLocality() {
        return locality == stl_thread;
    }

    List<Param> getParams();

    bool isArray, nullType, local, isEnum;
    bool resolved;
    bool prototype;
    Method* proto;
    FieldType returnType;
    double constant_value;
    RuntimeNote note;
    FieldType type;
    ClassObject* klass;
    List<Param> params; // for prototypes
    Ast *ast; // for parsing a generic field later
    int64_t serial, address;
    string name, fullName;
    ClassObject* owner;
    string key; // generic identifier
    List<AccessModifier> modifiers; // 3 max modifiers
    StorageLocality locality;
    int64_t thread_address;
};

#endif //SHARP_FIELD_H
