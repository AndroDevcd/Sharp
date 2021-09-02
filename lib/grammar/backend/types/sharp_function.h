//
// Created by BNunnally on 9/1/2021.
//

#ifndef SHARP_SHARP_FUNCTION_H
#define SHARP_SHARP_FUNCTION_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../dependency/dependancy.h"
#include "../meta_data.h"
#include "../access_flag.h"
#include "sharp_type.h"

struct sharp_class;
struct sharp_field;

enum function_type {
    normal_function = 1,
    constructor_function = 2,
    initializer_function = 3,
    operator_function = 4,
    delegate_function = 5,
    undefined_function = 0
};

struct sharp_function {
    sharp_function()
    :
        name(""),
        fullName(""),
        owner(NULL),
        implLocation(),
        dependencies(),
        flags(flag_none),
        ast(NULL),
        parameters(),
        returnType(),
        type(undefined_function)
    {}

    string name;
    string fullName;
    sharp_class *owner;
    impl_location implLocation;
    List<dependency> dependencies;
    List<sharp_field*> parameters;
    sharp_type returnType;
    function_type type;
    uInt flags;
    Ast* ast;
};

bool is_fully_qualified_function(sharp_function*);
bool function_parameters_match(List<sharp_field*>&, List<sharp_field*>&, bool);


#endif //SHARP_SHARP_FUNCTION_H
