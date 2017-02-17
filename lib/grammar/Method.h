//
// Created by BraxtonN on 1/31/2017.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H

#include <list>
#include "../../stdimports.h"
#include "Param.h"

class ClassObject;

class Method {

public:
    Method(string name, string module, ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
           ClassObject* rtype, RuntimeNote note)
    :
            name(name),
            pklass(klass),
            rType(rtype),
            n_rType(fnof),
            module(module),
            note(note)
    {
        this->params = new list<Param>();

        for(Param &param : params){
            this->params->push_back(param);
        }
    }

    Method(string name, string module, ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
           NativeField rtype, RuntimeNote note)
            :
            name(name),
            pklass(klass),
            n_rType(rtype),
            rType(NULL),
            module(module),
            note(note)
    {
        this->params = new list<Param>();

        for(Param &param : params){
            this->params->push_back(param);
        }
    }

    ClassObject* getParentClass() { return pklass; }
    ClassObject* getReturnType() { return rType; }
    string getName() { return name; }
    string getModule() { return module; }
    size_t paramCount() { return params->size(); }
    list<Param>* getParams() { return params; }
    Param getParam(int p) { return *std::next(params->begin(), p); }
    void clear() {
        modifiers.clear();

        for(Param& param : *params) {
            param.free();
        }
        std::free(params); params = NULL;
    }

    RuntimeNote note;
    uint64_t vaddr;
private:
    list<AccessModifier> modifiers; // 3 max modifiers
    ClassObject* pklass;
    string name;
    string module;
    list<Param>* params;
    ClassObject* rType;
    NativeField n_rType;
};


#endif //SHARP_METHOD_H
