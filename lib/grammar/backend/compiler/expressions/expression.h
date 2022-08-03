//
// Created by BNunnally on 9/17/2021.
//

#ifndef SHARP_EXPRESSION_H
#define SHARP_EXPRESSION_H

#include "../../types/sharp_type.h"
#include "../../operation/operation.h"

struct sharp_file;

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
            type(),
            scheme()
    {
        type.copy(t);
    }

    ~expression() {
        scheme.free();
    }


    sharp_type type;
    operation_schema scheme;
};

void compile_expression(expression &, Ast*);
void compile_expression_for_type(expression &, Ast*);

void convert_expression_type_to_real_type(
        expression &typeDefinition);

/**
 * This function can be used to call simple constructors or init() calls
 * @param ast
 * @param constructor
 * @param e
 * @param scheme
 */
void compile_initialization_call(
        Ast *ast,
        sharp_function *constructor,
        expression &e,
        operation_schema *scheme);

void compile_class_function_overload(
        sharp_class *with_class,
        expression &e,
        List<sharp_field*> &params,
        List<operation_schema*> &paramOperations,
        string &op,
        Ast *ast);

void compile_function_call(
        operation_schema *scheme,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        sharp_function *callee,
        bool isStaticCall,
        bool isPrimaryClass,
        bool isFunctionPointer);

void compile_initialization_call(
        Ast *ast,
        sharp_class *with_class,
        sharp_function *constructor,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        operation_schema *scheme);

#endif //SHARP_EXPRESSION_H
