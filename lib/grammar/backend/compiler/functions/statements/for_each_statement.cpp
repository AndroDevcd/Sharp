//
// Created by bknun on 6/12/2022.
//

#include "for_each_statement.h"
#include "../../../types/types.h"
#include "../../expressions/expression.h"
#include "../function_compiler.h"

void compile_for_each_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    stringstream ss;
    string std = "std";
    sharp_class *loopableClass = resolve_class(get_module(std), "loopable", true, false); // todo: fix bug later where 2 interfaces have the same function signatire and is inherited by a class and 1 function is valid for both interfaces
    sharp_field *iteratorField = NULL, *indexField = NULL, *resultField = NULL;
    sharp_function *initializer_function = NULL;
    sharp_label *beginLabel, *endLabel;
    uInt iterator_match_result = 0;
    sharp_type fieldType;
    uInt flags = flag_public;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_for_each;

    set_internal_label_name(ss, "for_each_begin", uniqueId++)
    beginLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "for_each_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);

    expression arrayExpr;
    compile_expression(arrayExpr, ast->getSubAst(ast_expression));

    if(!get_real_type(arrayExpr.type).isArray
        && !(get_class_type(arrayExpr.type) != NULL && inherits_generic_class(get_class_type(arrayExpr.type), loopableClass))) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col,
                               " foreach expression`" + type_to_str(arrayExpr.type) + "` must be an array type.");
    }

    if(!get_real_type(arrayExpr.type).isArray && get_class_type(arrayExpr.type) != NULL
        && inherits_generic_class(get_class_type(arrayExpr.type), loopableClass)) {
        List<sharp_field*> params;
        List<operation_schema*> paramOperations;
        sharp_function *get_elements = resolve_function("get_elements", get_class_type(arrayExpr.type), params, normal_function, 0, ast, false, false);

        if(get_elements != NULL) {
            if(check_flag(get_elements->flags, flag_static)) {
                current_file->errors->createNewError(GENERIC, ast->line, ast->col,
                     " could not call static function `" + function_to_str(get_elements) + "` in foreach on expression `" + type_to_str(arrayExpr.type) + "` via `loopable<>` interface.");
            }

            if(!get_elements->returnType.isArray) {
                current_file->errors->createNewError(GENERIC, ast->line, ast->col,
                                       " support function `" + function_to_str(get_elements) + "` must return an array.");
            }

            compile_function_call(&arrayExpr.scheme, params, paramOperations, get_elements, false, false,
                                  false);
            arrayExpr.type.copy(get_elements->returnType);
        } else {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col,
                   " could not locate support function `get_elements()` inherited from expression`" + type_to_str(arrayExpr.type) + "` via the `loopable<>` interface.");
        }
    }


    if(ast->getSubAst(ast_utype_arg)->hasSubAst(ast_utype)) {
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();
        fieldType.copy(resolve(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype)));

        if(fieldType.isArray) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` cannot evaluate to an array");
        }

        iteratorField = create_local_field(current_file, &current_context, name, flags, fieldType, normal_field, ast->getSubAst(ast_utype_arg));

        fieldType.isArray = true; // this type has already been consumed so we can modify it now
        fieldType.nullable = arrayExpr.type.nullable;
        if((iterator_match_result = is_implicit_type_match(fieldType, arrayExpr.type, match_operator_overload, initializer_function)) == no_match_found) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col,
                                   " incompatible types, cannot convert `" + type_to_str(arrayExpr.type) + "` to `" +
                                           type_to_str(fieldType) + "`.");
        }
    } else {
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();

        fieldType.copy(get_real_type(arrayExpr.type));
        fieldType.isArray = false;
        fieldType.nullable = false;
        iteratorField = create_local_field(current_file, &current_context, name, flags, fieldType, normal_field, ast->getSubAst(ast_utype_arg));
        iterator_match_result = direct_match;
    }

    set_internal_variable_name(ss, "foreach_index", uniqueId++)
    fieldType = sharp_type(type_int64);
    indexField =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);

    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, *subScheme,
         create_get_integer_constant_operation(&scheme_0, -1);
         ALLOCATE_REGISTER_1X(0, &scheme_0,
             create_retain_numeric_value_operation(&scheme_0, register_0);
             create_local_field_access_operation(&scheme_0, indexField);
             create_get_value_from_register_operation(&scheme_0, register_0);
         )
    )

    set_internal_variable_name(ss, "foreach_array_result", uniqueId++)
    fieldType.copy(arrayExpr.type);
    resultField =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);

    APPLY_TEMP_SCHEME_WITH_TYPE(1, scheme_assign_value, *subScheme,
        create_get_value_operation(&scheme_1, &arrayExpr.scheme);
        create_push_to_stack_operation(&scheme_1);
        create_local_field_access_operation(&scheme_1, resultField);
        create_pop_value_from_stack_operation(&scheme_1);
    )

    create_set_label_operation(subScheme, beginLabel);

    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_increment_for_index_value, *subScheme,
        create_local_field_access_operation(&scheme_0, indexField);
        create_increment_operation(&scheme_0, indexField->type.type);
    )

    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_for_each_position_check, *subScheme,
        ALLOCATE_REGISTER_2X(0, 1, &scheme_0,
            APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
               scheme_1.schemeType = scheme_access_local_field;
               create_local_field_access_operation(&scheme_1, resultField);
            )

            APPLY_TEMP_SCHEME_WITHOUT_INJECT(2,
                scheme_2.schemeType = scheme_access_local_field;
                create_local_field_access_operation(&scheme_2, indexField);
            )

            create_get_value_operation(&scheme_0, &scheme_2, false);
            create_retain_numeric_value_operation(&scheme_0, register_0);

            create_sizeof_operation(&scheme_0, &scheme_1);
            create_retain_numeric_value_operation(&scheme_0, register_0);
            create_lt_operation(&scheme_0, register_0, register_1);
            create_jump_if_false_operation(&scheme_0,  endLabel);
        )
    )

    if(iterator_match_result == match_initializer || iterator_match_result == match_constructor) {

        APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
            scheme_0.schemeType = scheme_get_constant;
            create_local_field_access_operation(&scheme_0, resultField);
            APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
                scheme_1.schemeType = scheme_get_constant;
                create_get_integer_constant_operation(&scheme_0, -1);
            )

            create_access_array_element_operation(&scheme_0, iteratorField->type, &scheme_1);
            create_push_to_stack_operation(&scheme_0);
        )

        expression e;
        e.type.copy(resultField->type);
        e.scheme.copy(scheme_0);

        APPLY_TEMP_SCHEME_WITH_TYPE(2, scheme_access_local_field, *subScheme,
            compile_initialization_call(ast, initializer_function, e, &scheme_2);
        )
    } else {
        APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_access_local_field, *subScheme,
             create_local_field_access_operation(&scheme_0, resultField);
             APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
                scheme_1.schemeType = scheme_get_constant;
                create_get_integer_constant_operation(&scheme_0, -1);
             )

             create_access_array_element_operation(&scheme_0, iteratorField->type, &scheme_1);
             create_push_to_stack_operation(&scheme_0);
        )

        if (iterator_match_result == direct_match) {
            APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_access_local_field, *subScheme,
                create_local_field_access_operation(&scheme_0, iteratorField);
                create_pop_value_from_stack_operation(&scheme_0);
            )
        } else if (iterator_match_result == indirect_match) {
            APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_access_local_field, *subScheme,
                create_local_field_access_operation(&scheme_0, iteratorField);
                create_pop_value_from_stack_operation(&scheme_0);
                create_cast_operation(&scheme_0, &iteratorField->type);
            )
        }
    }

    compile_block(ast->getSubAst(ast_block), subScheme, for_each_block, beginLabel, endLabel);
    current_context.blockInfo.reachable = true;

    create_jump_operation(subScheme, beginLabel);
    create_set_label_operation(subScheme, endLabel);

    add_scheme_operation(scheme, subScheme);
}
