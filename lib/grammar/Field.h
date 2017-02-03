//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../stdimports.h"
#include "NativeField.h"
#include "AccessModifier.h"
#include <list>

class ClassObject;

class Field {
public:
    Field(NativeField nf, uint64_t uid, string name, ClassObject* parent, list<AccessModifier>* modifiers)
    :
            nf(nf),
            uid(uid),
            name(name),
            parent(parent),
            klass(NULL),
            templName("")
    {
        this->modifiers = new list<AccessModifier>();
        if(modifiers == NULL)
            this->modifiers = NULL;
        else
            *this->modifiers = *modifiers;
    }

    Field(ClassObject* klass, uint64_t uid, string name, ClassObject* parent, list<AccessModifier>* modifiers)
            :
            nf(fnof),
            uid(uid),
            name(name),
            parent(parent),
            klass(klass),
            templName("")
    {
        this->modifiers = new list<AccessModifier>();
        if(modifiers == NULL)
            this->modifiers = NULL;
        else
            *this->modifiers = *modifiers;
    }

    /* For template Fields */
    Field(string templName, uint64_t uid, string name, ClassObject* parent, list<AccessModifier>* modifiers)
            :
            nf(fnof),
            uid(uid),
            name(name),
            parent(parent),
            klass(NULL),
            templName(templName)
    {
        this->modifiers = new list<AccessModifier>();
        if(modifiers == NULL)
            this->modifiers = NULL;
        else
            *this->modifiers = *modifiers;
    }

    Field()
            :
            nf(fnof),
            uid(0),
            name(""),
            modifiers(NULL)
    {
    }

    void clear() {
        klass = NULL;
        parent = NULL;
        if(modifiers != NULL) std::free(modifiers);
        modifiers = NULL;
    }

    bool isTempl() { return templName != ""; }

    bool operator==(const Field& f);

    void operator=(const Field& f)
    {
        clear();

        nf = f.nf;
        klass = f.klass;
        uid = f.uid;
        name = f.name;
        parent = f.parent;
        modifiers = f.modifiers;
    }

    NativeField nf;
    ClassObject* klass;
    uint64_t uid;
    string name;
    string templName;
    ClassObject* parent;
    list<AccessModifier>* modifiers; // 3 max modifiers
};


#endif //SHARP_FIELD_H
