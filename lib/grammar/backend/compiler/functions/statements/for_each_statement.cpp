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
    sharp_class *loopableClass = resolve_class(get_module(std), "loopable", true, false);
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


    create_block(&current_context, normal_block);
    if(ast->getSubAst(ast_utype_arg)->hasSubAst(ast_utype)) {
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();
        fieldType.copy(resolve(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype)));

        if(fieldType.isArray) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` cannot evaluate to an array");
        }

        iteratorField = create_local_field(current_file, &current_context, name, flags, fieldType, normal_field, ast->getSubAst(ast_utype_arg));

        sharp_type tmp(get_real_type(arrayExpr.type));
        tmp.isArray = false;
        tmp.nullable = tmp.nullableItems;

        if(tmp.type == type_object && get_class_type(fieldType)) {
            iterator_match_result = indirect_match;
        } else if((iterator_match_result = is_implicit_type_match(fieldType, tmp, match_operator_overload, initializer_function)) == no_match_found) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col,
                                   " incompatible types, cannot convert `" + type_to_str(tmp) + "` to `" +
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
    fieldType = sharp_type(type_var);
    indexField =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);

    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, *subScheme,
         operation_schema *indexValueScheme = new operation_schema();
         operation_schema *indexVariableScheme = new operation_schema();
         create_get_integer_constant_operation(indexValueScheme, -1);
         create_get_value_operation(&scheme_0, indexValueScheme, false, false);
         create_push_to_stack_operation(&scheme_0);

         create_local_field_access_operation(indexVariableScheme, indexField);
         create_get_value_operation(&scheme_0, indexVariableScheme, false, false);
         create_pop_value_from_stack_operation(&scheme_0);
         create_unused_data_operation(&scheme_0);

         delete indexValueScheme;
         delete indexVariableScheme;
    )

    set_internal_variable_name(ss, "foreach_array_result", uniqueId++)
    fieldType.copy(get_real_type(arrayExpr.type));
    resultField =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);


    APPLY_TEMP_SCHEME_WITH_TYPE(0, scheme_assign_value, *subScheme,
         operation_schema *resultVariableScheme = new operation_schema();
         create_get_value_operation(&scheme_0, &arrayExpr.scheme, false, false);
         create_push_to_stack_operation(&scheme_0);

         create_local_field_access_operation(resultVariableScheme, resultField);
         create_get_value_operation(&scheme_0, resultVariableScheme, false, false);
         create_pop_value_from_stack_operation(&scheme_0);
         create_unused_data_operation(&scheme_0);

         delete resultVariableScheme;
    )

    create_set_label_operation(subScheme, beginLabel);

    APPLY_TEMP_SCHEME(4, *subScheme,
        create_local_field_access_operation(&scheme_4, indexField);
        create_increment_operation(&scheme_4, indexField->type.type);

        scheme_4.schemeType = scheme_increment_for_index_value;
    )

    {
        APPLY_TEMP_SCHEME(0, *subScheme,
            ALLOCATE_REGISTER_2X(0, 1, &scheme_0,
                   APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
                       scheme_1.schemeType = scheme_access_local_field;
                       create_local_field_access_operation(&scheme_1, resultField);
                   )

                   // Note(Hack): because we incremented the indexField previously, we still hold the value so we don't need to get it again
                   create_retain_numeric_value_operation(&scheme_0, register_0);

                   APPLY_TEMP_SCHEME(3, scheme_0,
                       create_sizeof_operation(&scheme_3, &scheme_1);
                   )

                   create_retain_numeric_value_operation(&scheme_0, register_1);
                   create_lt_operation(&scheme_0, register_0, register_1);
                   create_jump_if_false_operation(&scheme_0, endLabel);
                   create_unused_data_operation(&scheme_0);

                   scheme_0.schemeType = scheme_for_each_position_check;
            )
        )
    }

    if(iterator_match_result == match_initializer || iterator_match_result == match_constructor) {
        create_dependency(initializer_function);

        APPLY_TEMP_SCHEME(0, *subScheme,
            create_local_field_access_operation(&scheme_0, resultField);
            APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
                scheme_1.schemeType = scheme_access_local_field;
                create_local_field_access_operation(&scheme_1, indexField);
            )

            sharp_type tmp(get_real_type(resultField->type));
            tmp.isArray = false;
            tmp.nullable = tmp.nullableItems;

            create_access_array_element_operation(&scheme_0, tmp, &scheme_1);

            expression e;

            e.type.copy(tmp);
            e.scheme.copy(scheme_0);
            scheme_0.free();

            APPLY_TEMP_SCHEME_WITHOUT_INJECT(5,
                compile_initialization_call(ast, initializer_function, e, &scheme_5);
            )

            operation_schema *iteratorVariableScheme = new operation_schema();
            create_get_value_operation(&scheme_0, &scheme_5, false, false);
            create_push_to_stack_operation(&scheme_0);

            create_local_field_access_operation(iteratorVariableScheme, iteratorField);
            create_get_value_operation(&scheme_0, iteratorVariableScheme, false, false);
            create_pop_value_from_stack_operation(&scheme_0);
            scheme_0.schemeType = scheme_assign_value;

            delete iteratorVariableScheme;
        )


    } else {
        APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
             scheme_0.schemeType = scheme_access_local_field;

             create_local_field_access_operation(&scheme_0, resultField);
             APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
                     scheme_1.schemeType = scheme_access_local_field;
                     create_local_field_access_operation(&scheme_1, indexField);
             )

             create_access_array_element_operation(&scheme_0, iteratorField->type, &scheme_1);
        )

        if (iterator_match_result == direct_match) {
            APPLY_TEMP_SCHEME_WITH_TYPE(6, scheme_assign_value, *subScheme,
                   operation_schema *iteratorVariableScheme = new operation_schema();
                   create_get_value_operation(&scheme_6, &scheme_0, false, false);
                   create_push_to_stack_operation(&scheme_6);

                   create_local_field_access_operation(iteratorVariableScheme, iteratorField);
                   create_get_value_operation(&scheme_6, iteratorVariableScheme, false, false);
                   create_pop_value_from_stack_operation(&scheme_6);

                   delete iteratorVariableScheme;
            )
        } else if (iterator_match_result == indirect_match) {
            APPLY_TEMP_SCHEME_WITH_TYPE(6, scheme_assign_value, *subScheme,
                   operation_schema *iteratorVariableScheme = new operation_schema();
                   create_get_value_operation(&scheme_6, &scheme_0, false, false);
                   create_push_to_stack_operation(&scheme_6);

                   create_local_field_access_operation(iteratorVariableScheme, iteratorField);
                   create_get_value_operation(&scheme_6, iteratorVariableScheme, false, false);
                   create_pop_value_from_stack_operation(&scheme_6);

                   if(get_class_type(iteratorField->type) != NULL)
                       create_cast_operation(&scheme_6, &iteratorField->type);

                   delete iteratorVariableScheme;
            )
        }
    }

    compile_block(ast->getSubAst(ast_block), subScheme, for_each_block, beginLabel, endLabel);
    delete_block();
    current_context.blockInfo.reachable = true;

    create_jump_operation(subScheme, beginLabel);
    create_set_label_operation(subScheme, endLabel);

    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}
