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
#include "../../../../compiler_info.h"
#include "post_ast_expression.h"

void compile_new_expression(expression *e, Ast *ast) {
    sharp_type newType = resolve(ast->getSubAst(ast_utype));

    if(ast->hasSubAst(ast_array_expression)) {
        compile_new_array_expression(&newType, e, ast->getSubAst(ast_array_expression));
    } else if(ast->hasSubAst(ast_vector_array)) {
        compile_new_vector_expression(&newType, e, ast->getSubAst(ast_vector_array));
    } else if(ast->hasSubAst(ast_expression_list)) {
        compile_new_class_expression(&newType, e, ast->getSubAst(ast_expression_list));
    }

    compile_post_ast_expression(e, ast, 2);
}

void compile_new_class_expression(sharp_type *newType, expression *e, Ast *ast) {
    List<expression*> expressions;
    List<sharp_field*> params;
    List<operation_scheme*> paramOperations;

    if(newType->type == type_class
       && check_flag(newType->_class->flags, flag_extension)) {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                 "cannot instantiate extension class `" + type_to_str(*newType) +"`.");
    }

    string name = "";
    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        expressions.add(new expression());
        compile_expression(*expressions.last(), ast->getSubAst(i));
        convert_expression_type_to_real_type(*expressions.last());

        impl_location location;
        params.add(new sharp_field(
                name, get_primary_class(&currThread->currTask->file->context), location,
                expressions.last()->type, flag_public, normal_field,
                ast
        ));

        paramOperations.add(new operation_scheme(expressions.last()->scheme));
    }

    if(newType->type == type_class) {
        sharp_function *constructor = resolve_function(get_simple_name(newType->_class), newType->_class,
                params, constructor_function,
                match_initializer | match_operator_overload,
                ast, false, true);

        if(constructor != NULL) {
            create_new_class_operation(&e->scheme, newType->_class);
            compile_function_call(&e->scheme, params,
                    paramOperations, constructor,
                    false, false);

            e->type.copy(*newType);
        } else {
            sharp_type returnType;
            string mock = get_simple_name(newType->_class);
            sharp_function mock_function(mock, newType->_class, impl_location(),
                                         flag_none, NULL, params, returnType, undefined_function, true);
            sharp_type unresolvedType(&mock_function);

            currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                    "cannot find constructor `" + type_to_str(unresolvedType) +"` for class `" + newType->_class->fullName + "`.");
        }
    } else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
               "cannot instantiate non-class type `" + type_to_str(*newType) +"`.");
    }

    deleteList(params);
    deleteList(paramOperations);
    deleteList(expressions);
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
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
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
                operation_scheme *arrayItemScheme = new operation_scheme(), resultScheme;
                arrayItemScheme->schemeType = scheme_new_class;
                arrayItemScheme->sc = newType->_class;

                arrayItemScheme->steps.add(
                        new operation_step(
                                operation_create_class,
                                e->type._class
                        )
                );

                List<operation_scheme*> scheme;
                scheme.add(arrayItemScheme);
                create_instance_function_call_operation(
                        &resultScheme, scheme, matchedConstructor);
                deleteList(scheme);
                setArrayItem->steps.add(new operation_step(operation_step_scheme, &resultScheme));
            }

            create_push_to_stack_operation(setArrayItem);
            create_assign_array_element_operation(setArrayItem, i);
        } else {
            currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
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
    operation_scheme *arraySieScheme = new operation_scheme(arraySize.scheme);

    e->type.copy(*newType);
    e->type.isArray = true;

    if(e->type.type == type_class)
        create_new_class_array_operation(&e->scheme, arraySieScheme, e->type._class);
    else if(e->type.type >= type_int8 && e->type.type <= type_var)
        create_new_number_array_operation(&e->scheme, arraySieScheme, e->type.type);
    else if(e->type.type == type_object)
        create_new_object_array_operation(&e->scheme, arraySieScheme);
    else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col,
                 "cannot create array of type `" + type_to_str(e->type) +"`.");
    }
}
