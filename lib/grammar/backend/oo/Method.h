//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H


#include "../DataEntity.h"
#include "Field.h"

enum function_type
{
    fn_normal,
    fn_constructor,
    fn_prototype,
    fn_op_overload,
    fn_delegate_impl,
    fn_delegate,
    fn_undefined
};

class Method : public DataEntity {
public:
    Method()
    :
        DataEntity(),
        fnType(fn_undefined),
        utype(NULL),
        overload(' ')
    {
        type = METHOD;
        params.init();
    }

    Method(string& name, string& module, ClassObject* owner, List<Field*> &params, List<AccessFlag> &flags, Meta &meta)
            :
            DataEntity(),
            fnType(fn_undefined),
            utype(NULL),
            overload(' ')
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

    function_type fnType;
    char overload;
    Utype* utype;
    List<Field*> params;
};


#endif //SHARP_METHOD_H
