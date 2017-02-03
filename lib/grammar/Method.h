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
    Method(string name, ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
           ClassObject* rtype)
    :
            name(name),
            pklass(klass),
            rType(rtype),
            n_rType(fnof),
            templName("")
    {
        this->params = new list<Param>();

        for(Param &param : params){
            this->params->push_back(param);
        }
    }

    Method(string name, ClassObject* klass, list<Param> params, list<AccessModifier> modifiers,
           NativeField rtype)
            :
            name(name),
            pklass(klass),
            n_rType(rtype),
            rType(NULL),
            templName("")
    {
        this->params = new list<Param>();

        for(Param &param : params){
            this->params->push_back(param);
        }
    }

    Method(string name, ClassObject* klass, list<Param> params, list<AccessModifier> modifiers, string tmplName)
            :
            name(name),
            pklass(klass),
            n_rType(fnof),
            rType(NULL),
            templName(tmplName)
    {
        this->params = new list<Param>();

        for(Param &param : params){
            this->params->push_back(param);
        }
    }

    ClassObject* getParentClass() { return pklass; }
    ClassObject* getReturnType() { return rType; }
    string getName() { return name; }
    size_t paramCount() { return params->size(); }
    list<Param>* getParams() { return params; }
    Param getParam(int p) { return *std::next(params->begin(), p); }
    bool isTemplFunction() { return templName != ""; }

private:
    list<AccessModifier> modifiers; // 3 max modifiers
    ClassObject* pklass;
    string name;
    string templName;
    list<Param>* params;
    ClassObject* rType;
    NativeField n_rType;
};


#endif //SHARP_METHOD_H
