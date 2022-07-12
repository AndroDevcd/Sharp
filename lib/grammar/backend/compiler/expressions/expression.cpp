//
// Created by BNunnally on 9/17/2021.
//

#include "expression.h"
#include "unary/minus_expression.h"
#include "../../dependency/dependancy.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"
#include "unary/pre_increment_expression.h"
#include "primary/binary_expression.h"
#include "../../postprocessor/field_processor.h"
#include "dictionary_expression.h"
#include "vector_array_expression.h"
#include "elvis_expression.h"
#include "assign_expression.h"
#include "binary/binary_expression.h"
#include "../../postprocessor/function_processor.h"
#include "../../../compiler_info.h"

void compile_expression(expression &e, Ast *ast) {
    if(ast->getType() == ast_expression)
        compile_expression(e, ast->getSubAst(0));
    else if(ast->getType() == ast_minus_e)
        compile_minus_expression(&e, ast);
    else if(ast->getType() == ast_pre_inc_e)
        compile_pre_increment_expression(&e, ast);
    else if(ast->getType() == ast_primary_expr)
        compile_primary_expression(&e, ast);
    else if(ast->getType() == ast_dictionary_array)
        compile_dictionary_expression(&e, ast);
    else if(ast->getType() == ast_vect_e)
        compile_vector_array_expression(&e, ast->getSubAst(ast_vector_array));
    else if(ast->getType() == ast_elvis_e)
        compile_elvis_expression(&e, ast);
    else if(ast->getType() == ast_assign_e)
        compile_assign_expression(&e, ast);
    else if(ast->getType() == ast_and_e
        || ast->getType() == ast_equal_e
        || ast->getType() == ast_less_e
        || ast->getType() == ast_shift_e
        || ast->getType() == ast_add_e
        || ast->getType() == ast_mult_e
        || ast->getType() == ast_exponent_e)
        compile_binary_expression(&e, ast);
}

void compile_expression_for_type(sharp_type &type, Ast *ast) {
    expression e;
    compile_expression(e, ast);

    type.copy(e.type);
}

void convert_expression_type_to_real_type(
        expression &typeDefinition) {
    if(typeDefinition.type == type_integer
       || typeDefinition.type == type_decimal) {
        typeDefinition.type = type_var;
    } else if(typeDefinition.type == type_char
              || typeDefinition.type == type_bool) {
        typeDefinition.type = type_int8;
    } else if(typeDefinition.type == type_string) {
        typeDefinition.type = type_int8;
        typeDefinition.type.isArray = true;
    } else if(typeDefinition.type == type_null) {
        typeDefinition.type = type_object;
    } else if(typeDefinition.type == type_field) {
        process_field(typeDefinition.type.field);
        typeDefinition.type.copy(typeDefinition.type.field->type);
    }
}

void compile_initialization_call(
        Ast *ast,
        sharp_function *constructor,
        expression &e,
        operation_schema *scheme) {
    string name = "";
    List<sharp_field *> params;
    List<operation_schema *> paramOperations;

    impl_location location;
    params.add(new sharp_field(
            name, get_primary_class(&currThread->currTask->file->context), location,
            e.type, flag_public, normal_field,
            ast
    ));
    paramOperations.add(new operation_schema(e.scheme));

    create_new_class_operation(scheme, get_class_type(current_context.functionCxt->returnType));
    compile_function_call(
            scheme, params,
            paramOperations, constructor,
            false,
            false);
}

void compile_initialization_call(
        Ast *ast,
        sharp_class *with_class,
        sharp_function *constructor,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        operation_schema *scheme) {
    create_new_class_operation(scheme, with_class);
    compile_function_call(
            scheme, params,
            paramOperations, constructor,
            false,
            false);
}

void compile_class_function_overload(
        sharp_class *with_class,
        expression &e,
        List<sharp_field*> &params,
        List<operation_schema*> &paramOperations,
        string &op,
        Ast *ast) {
    sharp_function *fun;

    if((fun = resolve_function(
            "operator" + op,
            with_class,
            params,
            operator_function,
            match_initializer | match_operator_overload,
            ast,
            true,
            true)) != NULL) {
        process_function_return_type(fun);

        compile_function_call(&e.scheme, params, paramOperations, fun, false, false);
        e.type.copy(fun->returnType);
    } else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
                "use of operator `" + op + "` does not have any qualified overloads with class `" + with_class->fullName + "`");

    }
}

void compile_function_call(
        operation_schema *scheme,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        sharp_function *callee,
        bool isStaticCall,
        bool isPrimaryClass) {
    if(scheme) {
        Int matchResult;
        List<operation_schema*> compiledParamOperations;
        for (Int i = 0; i < params.size(); i++) {
            sharp_type *asignee = &callee->parameters.get(i)->type;
            sharp_type *assigner = &params.get(i)->type;
            sharp_function *matchedConstructor = NULL;

            if(assigner->type == type_get_component_request) {
                paramOperations.get(i)->copy(
                        *assigner->componentRequest->resolvedTypeDefinition->scheme);
                matchResult = direct_match;
            } else {
                matchResult = is_explicit_type_match(*asignee, *assigner);
                if (matchResult == no_match_found) {
                    matchResult = is_implicit_type_match(
                            *asignee, *asignee, 0,
                            matchedConstructor);
                }
            }


            compiledParamOperations.add(new operation_schema());
            create_function_parameter_push_operation(
                    asignee,
                    matchResult,
                    matchedConstructor,
                    paramOperations.get(i),
                    compiledParamOperations.last()
            );
        }

        if (isStaticCall) {
            create_static_function_call_operation(scheme, compiledParamOperations, callee);
        } else {
            if (isPrimaryClass)
                create_primary_class_function_call_operation(scheme, compiledParamOperations, callee);
            else {
                create_instance_function_call_operation(scheme, compiledParamOperations, callee);
            }
        }
    }
}
