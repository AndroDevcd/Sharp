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
            n_rType(fnof)
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
            rType(NULL)
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

private:
    list<AccessModifier> modifiers; // 3 max modifiers
    ClassObject* pklass;
    string name;
    list<Param>* params;
    ClassObject* rType;
    NativeField n_rType;
};


#endif //SHARP_METHOD_H
