//
// Created by BNunnally on 9/29/2021.
//

#include "new_expression.h"
#include "../../../types/sharp_type.h"
#include "../../../dependency/dependancy.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../postprocessor/field_processor.h"
#include "../../../types/types.h"

void compile_new_expression(expression *e, Ast *ast) {
    sharp_type newType = resolve(ast->getSubAst(ast_utype));

    if(ast->hasSubAst(ast_array_expression)) {
        compile_new_array_expression(&newType, e, ast->getSubAst(ast_array_expression));
    } else if(ast->hasSubAst(ast_vector_array)) {
        compile_new_vector_expression(&newType, e, ast->getSubAst(ast_vector_array));
    }
}

void compile_new_vector_expression(sharp_type *newType, expression *e, Ast *ast) {
    operation_scheme *arraySizeScheme = new operation_scheme();
    arraySizeScheme->schemeType = scheme_get_constant;
    arraySizeScheme->steps.add(new operation_step(operation_get_integer_constant, (Int)ast->getSubAstCount()));

    if(newType->type == type_class)
        create_new_class_array_operation(&e->scheme, arraySizeScheme, e->type._class);
    else if(newType->type >= type_int8 && newType->type <= type_var)
        create_new_number_array_operation(&e->scheme, arraySizeScheme, e->type.type);
    else if(newType->type == type_object)
        create_new_object_array_operation(&e->scheme, arraySizeScheme);
    else {
        currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                                                           "cannot create array of type `" + type_to_str(e->type) +"`.");
    }

    Int matchResult;
    create_push_to_stack_operation(&e->scheme);
    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        expression *expr = new expression();
        sharp_function *matchedConstructor = NULL;
        compile_expression(*expr, ast->getSubAst(i));
        operation_scheme *setArrayItem = new operation_scheme();
        setArrayItem->schemeType = scheme_get_array_value;

        if(expr->type.type == type_integer
           || expr->type.type == type_decimal) {
            expr->type.type = type_var;
        } else if(expr->type.type == type_char
                  || expr->type.type == type_bool) {
            expr->type.type = type_int8;
        } else if(expr->type.type == type_string) {
            expr->type.type = type_int8;
        } else if(expr->type.type == type_null) {
            expr->type.type = type_object;
        } else if(expr->type.type == type_field) {
            process_field(expr->type.field);
            expr->type.copy(expr->type.field->type);
        } else if(expr->type.type == type_nil) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                         "type of `nil` cannot be defined as array element.");

        } else if(expr->type.type == type_lambda_function) {
            if(!is_fully_qualified_function(expr->type.fun)) {
                currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                             "lambda must be fully qualified.");
            }

            expr->type.type = type_function_ptr;
        }

        matchResult = is_explicit_type_match(*newType, expr->type);
        if (matchResult == no_match_found) {
            matchResult = is_implicit_type_match(
                    *newType, expr->type,
                    match_initializer | match_operator_overload,
                    matchedConstructor);

        }

        if(matchResult != no_match_found) {
            if(matchResult == match_normal) {
                setArrayItem->steps.add(new operation_step(operation_get_value, &expr->scheme));
            } else { // match_constructor
                operation_scheme arrayItemScheme, resultScheme;
                arrayItemScheme.schemeType = scheme_new_class;
                arrayItemScheme.sc = newType->_class;

                arrayItemScheme.steps.add(
                        new operation_step(
                                operation_create_class,
                                e->type._class
                        )
                );

                List<operation_scheme> scheme;
                scheme.add(arrayItemScheme);
                create_instance_function_call_operation(
                        &resultScheme, scheme, matchedConstructor);
                scheme.free();
                setArrayItem->steps.add(new operation_step(operation_step_scheme, &resultScheme));
            }

            create_push_to_stack_operation(setArrayItem);
            create_assign_array_element_operation(setArrayItem, i);
        } else {
            currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                         "cannot assign array item of type `" + type_to_str(e->type) +"` to array of type `" + type_to_str(*newType) + "`.");
        }

        e->scheme.steps.add(new operation_step(operation_assign_array_value, setArrayItem));
        delete expr;
    }

    e->type.copy(*newType);
    e->type.isArray = true;
    create_pop_value_from_stack_operation(&e->scheme);
}

void compile_new_array_expression(sharp_type *newType, expression *e, Ast *ast) {
    expression arraySize;
    compile_expression(*e, ast);

    e->type.copy(*newType);
    e->type.isArray = true;

    if(e->type.type == type_class)
        create_new_class_array_operation(&e->scheme, &arraySize.scheme, e->type._class);
    else if(e->type.type >= type_int8 && e->type.type <= type_var)
        create_new_number_array_operation(&e->scheme, &arraySize.scheme, e->type.type);
    else if(e->type.type == type_object)
        create_new_object_array_operation(&e->scheme, &arraySize.scheme);
    else {
        currThread->currTask->file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col,
                 "cannot create array of type `" + type_to_str(e->type) +"`.");
    }
}
