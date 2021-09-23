//
// Created by BNunnally on 9/17/2021.
//

#ifndef SHARP_EXPRESSION_H
#define SHARP_EXPRESSION_H

#include "../../types/sharp_type.h"
#include "../../operation/operation.h"

struct expression {
    expression()
    :
        type(),
        scheme()
    {}

    expression(const expression &e)
            :
            type(e.type),
            scheme(e.scheme)
    {}

    expression(sharp_type t)
            :
            type(t),
            scheme()
    {}

    ~expression() {
        scheme.free();
    }


    sharp_type type;
    operation_scheme scheme;
};

void compile_expression(expression &, Ast*);
void compile_expression_for_type(expression &, Ast*);

void compile_class_function_overload(
        sharp_class *with_class,
        expression &e,
        List<sharp_field*> &params,
        List<operation_scheme> &paramOperations,
        string &op,
        Ast *ast);

void compile_function_call(
        operation_scheme *scheme,
        List<sharp_field*> & params,
        List<operation_scheme> &paramOperations,
        sharp_function *callee,
        bool isStaticCall,
        bool isPrimaryClass)

#endif //SHARP_EXPRESSION_H
