//
// Created by BNunnally on 9/19/2021.
//

#ifndef SHARP_UNRESOLVED_EXTENSION_FUNCTION_H
#define SHARP_UNRESOLVED_EXTENSION_FUNCTION_H

#include "../../../../stdimports.h"
#include "../../frontend/parser/Ast.h"
#include "../types/function_type.h"

struct unresolved_extension_function {
    unresolved_extension_function()
    :
        name(""),
        ast(NULL),
        type(undefined_function)
    {}

    unresolved_extension_function(const unresolved_extension_function &uef)
            :
            name(uef.name),
            ast(uef.ast),
            type(uef.type)
    {}
    unresolved_extension_function(string &name, Ast *ast, function_type type)
            :
            name(name),
            ast(ast),
            type(type)
    {}


    string name;
    function_type type;
    Ast *ast;
};


#endif //SHARP_UNRESOLVED_EXTENSION_FUNCTION_H
