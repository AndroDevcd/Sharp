//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H


#include "../data/DataEntity.h"
#include "Field.h"
#include "FunctionType.h"
#include "../code/CodeHolder.h"

struct CodeData {
    CodeData()
    :
        code()
    {
    }

    CodeHolder code;
};

class Method : public DataEntity {
public:
    Method()
    :
        DataEntity(),
        fnType(fn_undefined),
        utype(NULL),
        overload(' '),
        extensionFun(false),
        delegateAddr(invalidAddr),
        data()
    {
        type = METHOD;
        params.init();
    }

    Method(string& name, string& module, ClassObject* owner, List<Field*> &params, List<AccessFlag> &flags, Meta &meta)
            :
            DataEntity(),
            fnType(fn_undefined),
            utype(NULL),
            overload(' '),
            extensionFun(false),
            data()
    {
        this->type = METHOD;
        this->params.init();
        this->name=name;
        this->module=module;
        this->owner=owner;
        this->flags.addAll(flags);
        this->params.addAll(params);
        this->meta.copy(meta);
    }

    void free();
    string toString();
    static string paramsToString(List<Field*> &params);
    void copy(Method *method) {
        DataEntity::copy(method);
        fnType = method->fnType;
        overload = method->overload;
        extensionFun = method->extensionFun;
        utype = method->utype;
        params.addAll(method->params);
    }

    function_type fnType;
    char overload;
    bool extensionFun;
    long delegateAddr;
    Utype* utype;
    List<Field*> params;
    CodeData data;
};


#endif //SHARP_METHOD_H
