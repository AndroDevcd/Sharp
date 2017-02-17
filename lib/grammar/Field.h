//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_FIELD_H
#define SHARP_FIELD_H

#include "../../stdimports.h"
#include "NativeField.h"
#include "AccessModifier.h"
#include "RuntimeNote.h"
#include <list>

class ClassObject;

class Field {
public:
    Field(NativeField nf, uint64_t uid, string name, ClassObject* parent, list<AccessModifier>* modifiers,
          RuntimeNote note)
    :
            nf(nf),
            uid(uid),
            name(name),
            parent(parent),
            klass(NULL),
            note(note),
            refrence(false),
            pointer(false)
    {
        this->modifiers = new list<AccessModifier>();
        if(modifiers == NULL)
            this->modifiers = NULL;
        else
            *this->modifiers = *modifiers;
    }

    Field(ClassObject* klass, uint64_t uid, string name, ClassObject* parent, list<AccessModifier>* modifiers,
          RuntimeNote note)
            :
            nf(fnof),
            uid(uid),
            name(name),
            parent(parent),
            klass(klass),
            note(note),
            refrence(false),
            pointer(false)
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
            modifiers(NULL),
            note("","",0,0),
            refrence(false),
            pointer(false)
    {
    }

    bool operator==(const Field& f);

    void operator=(const Field& f)
    {
        free();

        nf = f.nf;
        klass = f.klass;
        uid = f.uid;
        name = f.name;
        parent = f.parent;
        modifiers = f.modifiers;
        refrence = f.refrence;
    }
    void free(){
        klass = NULL;
        parent = NULL;

        if(modifiers != NULL) {
            modifiers->clear();
            std::free (modifiers);
        }
        modifiers = NULL;
    }

    bool refrence, pointer;
    RuntimeNote note;
    NativeField nf;
    ClassObject* klass;
    uint64_t uid;
    string name;
    ClassObject* parent;
    list<AccessModifier>* modifiers; // 3 max modifiers
};


#endif //SHARP_FIELD_H
