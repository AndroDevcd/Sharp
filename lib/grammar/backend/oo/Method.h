//
// Created by BraxtonN on 10/22/2019.
//

#ifndef SHARP_METHOD_H
#define SHARP_METHOD_H


#include "../DataEntity.h"
#include "Field.h"

enum function_type
{
    fn_constructor,
    fn_undefined
};

class Method : public DataEntity {
public:
    Method()
    :
        DataEntity(),
        type(fn_undefined),
        generic(false)
    {
        params.init();
    }

    Method(string& name, string& module, ClassObject* owner, List<Field*> params, List<AccessFlag> flags, Meta &meta)
            :
            DataEntity(),
            generic(false)
    {
        this->params.init();
        this->name=name;
        this->module=module;
        this->owner=owner;
        this->flags.addAll(flags);
        this->params.addAll(params);
        this->meta.copy(meta);
    }

    void free();


    bool generic;
    function_type type;
    List<Field*> params;
};


#endif //SHARP_METHOD_H
