//
// Created by BNunnally on 5/18/2020.
//

#ifndef SHARP_SYMBOL_H
#define SHARP_SYMBOL_H

#include "../../../stdimports.h"
#include "string.h"

enum data_type { // todo: develop a "core" folder that will host anything that is chared between the 2 codebases
    type_int8=0,
    type_int16,
    type_int32,
    type_int64,
    type_uint8,
    type_uint16,
    type_uint32,
    type_uint64,
    type_function_ptr,
    type_var,
    type_object,
    type_class,
    type_field,
    type_function,
    type_lambda_function,
    type_module,
    type_import_group,
    type_null,
    type_nil,
    type_any,
    type_untyped,
    type_undefined,
    type_integer,
    type_char,
    type_string,
    type_bool,
    type_decimal,
    type_get_component_request,
    type_label,
};

extern int invalidAddr;
class ClassObject;
class Symbol {
public:
    Symbol()
    {
        init();
    }

    void init() {
        address = invalidAddr;
        guid = invalidAddr;
        owner = NULL;
        flags = 0;
        type = type_undefined;
        new (&name)string();
        new (&fullName)string();
    }

    void free() {
        name.clear();
        fullName.clear();
    }

    int32_t address;
    int32_t guid;
    string name;
    string fullName;
    ClassObject *owner;
    uint32_t flags;
    data_type type;
};

#endif //SHARP_SYMBOL_H
