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

class Field {
public:
    Field(FieldType type, uint64_t serial, const string name, ClassObject* parent, List<AccessModifier>& modifiers,
          RuntimeNote& note)
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
            local(false)
    {
        this->modifiers.init();
        this->modifiers.addAll(modifiers);
    }

    Field(ClassObject* klass, uint64_t serial, const string name, ClassObject* parent, List<AccessModifier>& modifiers,
          RuntimeNote& note)
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
            local(false)
    {
        this->modifiers.init();
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
            owner(NULL)
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
    }

    void free(){
        klass = NULL;
        owner = NULL;

        name.clear();
        fullName.clear();
        modifiers.free();
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

    bool isArray, nullType, local;
    bool resolved;
    RuntimeNote note;
    FieldType type;
    ClassObject* klass;
    int64_t serial, address;
    string name, fullName;
    ClassObject* owner;
    List<AccessModifier> modifiers; // 3 max modifiers
};

#endif //SHARP_FIELD_H
