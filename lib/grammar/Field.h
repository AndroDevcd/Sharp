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
            note(note)
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
            note(note)
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
            note("","",0,0)
    {
    }

    void clear() {
        klass = NULL;
        parent = NULL;
        if(modifiers != NULL) std::free(modifiers);
        modifiers = NULL;
    }

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
    void free(){
        if(modifiers == NULL) return;

        modifiers->clear();
        std::free(modifiers); modifiers = NULL;
    }

    RuntimeNote note;
    NativeField nf;
    ClassObject* klass;
    uint64_t uid;
    string name;
    ClassObject* parent;
    list<AccessModifier>* modifiers; // 3 max modifiers
};


#endif //SHARP_FIELD_H
