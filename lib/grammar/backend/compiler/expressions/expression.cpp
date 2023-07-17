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
#include "inline_if_expression.h"
#include "primary/force_non_null_expression.h"

void compile_expression(expression &e, Ast *ast, Int endLabel) {
    if(ast->getType() == ast_expression)
        compile_expression(e, ast->getSubAst(0));
    else if(ast->getType() == ast_minus_e)
        compile_minus_expression(&e, ast);
    else if(ast->getType() == ast_pre_inc_e)
        compile_pre_increment_expression(&e, ast);
    else if(ast->getType() == ast_primary_expr)
        compile_primary_expression(&e, ast, endLabel);
    else if(ast->getType() == ast_dictionary_array)
        compile_dictionary_expression(&e, ast);
    else if(ast->getType() == ast_vect_e)
        compile_vector_array_expression(&e, ast->getSubAst(ast_vector_array));
    else if(ast->getType() == ast_force_non_null_e)
        compile_force_non_null_expression(&e, ast);
    else if(ast->getType() == ast_elvis_e)
        compile_elvis_expression(&e, ast);
    else if(ast->getType() == ast_ques_e)
        compile_inline_if_expression(&e, ast);
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

bool pre_initialize_class(sharp_class *sc) {
    return sc->fullName == "std#bool" ||
       sc->fullName == "std#byte" ||
       sc->fullName == "std#short" ||
       sc->fullName == "std#char" ||
       sc->fullName == "std#int" ||
       sc->fullName == "std#long" ||
       sc->fullName == "std#ushort" ||
       sc->fullName == "std#uchar" ||
       sc->fullName == "std#uint" ||
       sc->fullName == "std#ulong" ||
       sc->fullName == "std#double" ||
       sc->fullName == "std#string";
}

void convert_expression_type_to_real_type(
        expression &typeDefinition) {
    if(typeDefinition.type == type_integer
       || typeDefinition.type == type_decimal) {
        typeDefinition.type = typeDefinition.type == type_decimal ? type_var : type_int32;
    } else if(typeDefinition.type == type_char
              || typeDefinition.type == type_bool) {
        typeDefinition.type = type_int8;
    } else if(typeDefinition.type == type_string) {
        typeDefinition.type = type_int8;
        typeDefinition.type.isArray = true;
    } else if(typeDefinition.type == type_field) {
        process_field(typeDefinition.type.field);
        bool nullable = typeDefinition.type.nullable || typeDefinition.type.field->type.nullable;
        typeDefinition.type.copy(typeDefinition.type.field->type);
        typeDefinition.type.nullable = nullable;
    }
}

void recompile_cond_expression(expression &out, Ast *ast) {
    out.scheme.free();
    out.type.free();
    compile_cond_expression(out, ast);
}

void extract_value_field_from_expression(expression &e, string className, Ast *ast, bool forceError) {
    sharp_class *sc = get_class_type(e.type);

    if(sc != NULL && !e.type.nullable && !e.type.isArray &&
        (
            (!className.empty() && sc->fullName == className)
            || (className.empty() && sc->fullName == "std#bool")
            || (className.empty() && sc->fullName == "std#byte")
            || (className.empty() && sc->fullName == "std#short")
            || (className.empty() && sc->fullName == "std#char")
            || (className.empty() && sc->fullName == "std#int")
            || (className.empty() && sc->fullName == "std#long")
            || (className.empty() && sc->fullName == "std#ushort")
            || (className.empty() && sc->fullName == "std#uchar")
            || (className.empty() && sc->fullName == "std#uint")
            || (className.empty() && sc->fullName == "std#ulong")
            || (className.empty() && sc->fullName == "std#double")
        )
    ) {
        sharp_field *field = resolve_field("value", sc, true);

        if(field != NULL) {
            APPLY_TEMP_SCHEME(1, e.scheme,
                 create_get_value_operation(&scheme_1, &e.scheme);
                 create_instance_field_access_operation(&scheme_1, field);
                 e.scheme.free();
            )

            e.type.copy(field->type);
        } else {
            create_new_error(COULD_NOT_RESOLVE, ast,
                             " field `" + className + ".value`  was not found.");
        }
    } else if(forceError && !is_evaluable_type(e.type)) {
        create_new_error(GENERIC,  ast, "expression must evaluate to a numeric value, true, or false. But type `" + type_to_str(e.type) + "` was found.");
    }
}

void compile_cond_expression(expression &e, Ast *ast) {
    compile_expression(e, ast);
    extract_value_field_from_expression(e, "std#bool", ast);
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
            name, get_primary_class(&currThread->currTask->file->ctx), location,
            e.type, flag_public, normal_field,
            ast
    ));
    paramOperations.add(new operation_schema(e.scheme));

    create_new_class_operation(scheme, constructor->owner);
    create_duplicate_operation(scheme);

    APPLY_TEMP_SCHEME(0, *scheme,
    compile_function_call(
            &scheme_0, params,
            paramOperations, constructor,
            false,
            false,
            false);
    )
}

void compile_initialization_call(
        Ast *ast,
        sharp_class *with_class,
        sharp_function *constructor,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        operation_schema *scheme) {
    create_new_class_operation(scheme, with_class);
    create_duplicate_operation(scheme);

    APPLY_TEMP_SCHEME(0, *scheme,
         compile_function_call(
                 &scheme_0, params,
                 paramOperations, constructor,
                 false,
                 false,
                 false);
    )
}

sharp_function* compile_class_function_overload(
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
        data_type prevType = fun->returnType.type;
        process_function_return_type(fun);

        compile_function_call(&e.scheme, params, paramOperations, fun, false, false,
                              false);

        create_dependency(fun);
        e.type.copy(fun->returnType);
        return fun;
    } else {
        create_new_error(GENERIC, ast,
                "use of operator `" + op + "` does not have any qualified overloads with class `" + with_class->fullName + "`");
        return NULL;
    }
}

void compile_function_call(
        operation_schema *scheme,
        List<sharp_field*> & params,
        List<operation_schema*> &paramOperations,
        sharp_function *callee,
        bool isStaticCall,
        bool isPrimaryClass,
        bool isFunctionPointer) {
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
                            *asignee, *assigner, constructor_only,
                            matchedConstructor);

                    if(matchedConstructor != NULL)
                        create_dependency( matchedConstructor);
                } else if(*assigner == type_lambda_function
                   && !is_fully_qualified_function(assigner->fun)) {
                    fully_qualify_function(assigner->fun, asignee->fun);
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

        create_dependency(callee);
        if(isFunctionPointer) {
            ALLOCATE_REGISTER_1X(1, scheme,
              create_dynamic_function_call_operation(scheme, compiledParamOperations, callee, register_1, false);
           )
        } else if (isStaticCall) {
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
