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
    Method(string name, ClassObject* klass, list<Param> params, AccessModifier modifiers[3],
           ClassObject* rtype)
    :
            name(name),
            pklass(klass),
            rType(rtype)
    {
        this->params = new list<Param>();

        this->modifiers[0] = modifiers[0];
        this->modifiers[1] = modifiers[1];
        this->modifiers[2] = modifiers[2];

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
    AccessModifier modifiers[3]; // 3 max modifiers
    ClassObject* pklass;
    string name;
    list<Param>* params;
    ClassObject* rType;
};


#endif //SHARP_METHOD_H
