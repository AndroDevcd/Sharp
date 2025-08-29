//
// Created by bknun on 7/2/2022.
//

#include "try_catch_statement.h"
#include "../../../types/types.h"
#include "../../compiler.h"
#include "../function_compiler.h"


sharp_class* compile_catch_clause(
        Ast *ast,
        operation_schema *scheme,
        sharp_tc_data *tc_data,
        bool *controlPaths) {
    catch_data_info *catchData = create_catch_data_tracker(tc_data, scheme);
    sharp_class *exceptionClass = NULL;

    sharp_type handlingClass = resolve(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype));
    if(ast->getSubAst(ast_utype_arg)->getTokenCount() > 0) {
        sharp_field *catchField;
        uInt flags = flag_public;
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();

        if(handlingClass.isArray) {
            create_new_error(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` cannot evaluate to an array");
        }

        if(get_class_type(handlingClass) == NULL) {
            create_new_error(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` must be of type class: `" +
                    type_to_str(handlingClass) + "`");
        } else {
            create_dependency(get_class_type(handlingClass));
            create_set_catch_class_operation(catchData, get_class_type(handlingClass), scheme);
        }

        catchField = create_local_field(current_file, &current_context, name, flags, handlingClass, normal_field, ast);
        create_set_catch_field_operation(catchData, catchField, scheme);
    } else {

        if(handlingClass.isArray) {
            create_new_error(GENERIC, ast->line, ast->col, "handling class  `" +  type_to_str(handlingClass) + "` cannot evaluate to an array");
        }

        if(get_class_type(handlingClass) == NULL) {
            create_new_error(GENERIC, ast->line, ast->col, "type `" +  type_to_str(handlingClass) + "` must be of type class");
        } else {
            create_set_catch_class_operation(catchData, get_class_type(handlingClass), scheme);
        }

        create_set_catch_field_operation(catchData, NULL, scheme);
    }

    exceptionClass = get_class_type(handlingClass);
    create_catch_start_operation(scheme, catchData);
    if(!compile_block(ast->getSubAst(ast_block), scheme, catch_block)) {
        controlPaths[CATCH_CONTROL_PATH] = false;
    }

    return exceptionClass;
}

void compile_try_catch_statement(Ast *ast, operation_schema *scheme, bool *controlPaths) {
    stringstream ss;
    string std = "std";
    sharp_tc_data *tc_data = NULL;
    sharp_field *exceptionObject = NULL;
    sharp_field *returnAddressField = NULL;
    List<sharp_class*> catchedClasses;
    List<operation_schema*> lockSchemes;
    sharp_label *finallyBeginLabel, *finallyEndLabel, *endLabel, *returnCheckEndLabel;
    operation_schema *subScheme = new operation_schema();

    set_internal_label_name(ss, "finally_begin", uniqueId++)
    finallyBeginLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "finally_end", uniqueId++)
    finallyEndLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "try_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "return_check_end", uniqueId++)
    returnCheckEndLabel = create_label(ss.str(), &current_context, ast, subScheme);

    sharp_class *throwable = resolve_class(get_module(std), "throwable", false, false);
    bool hasFinallyBlock = ast->hasSubAst(ast_finally_block);
    bool hasCatchBlock = ast->hasSubAst(ast_catch_clause);

    if(hasFinallyBlock) {
        uInt flags = flag_public;
        set_internal_variable_name(ss, "exception_object", 0)
        if((exceptionObject = resolve_local_field(ss.str(), &current_context)) == NULL) {
            sharp_type fieldType = sharp_type(type_object);
            exceptionObject =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);
            create_dependency(exceptionObject);
        }

        set_internal_variable_name(ss, "return_address", 0)
        if((returnAddressField = resolve_local_field(ss.str(), &current_context)) == NULL) {
            sharp_type fieldType = sharp_type(type_var);
            returnAddressField =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);
            create_dependency(returnAddressField);
        }
    }

    tc_data = create_try_catch_data_tracker(subScheme);
    create_try_catch_start_operation(subScheme, tc_data);
    create_try_catch_block_start_operation(subScheme, tc_data);

    if(hasFinallyBlock)
        create_setup_local_field_operation(subScheme, returnAddressField);

    controlPaths[TRY_CONTROL_PATH] =
            compile_block(ast->getSubAst(ast_block), subScheme, try_block,
                          NULL, NULL, NULL, ast->hasSubAst(ast_finally_block) ? finallyBeginLabel : NULL);
    create_try_catch_block_end_operation(subScheme, tc_data);

    if(hasCatchBlock)
        create_jump_operation(subScheme, endLabel);

    controlPaths[CATCH_CONTROL_PATH] = true;
    for(Int i = 1; i < ast->getSubAstCount(); i++) {
        Ast *branch = ast->getSubAst(i);

        current_context.blockInfo.reachable = true;
        switch (branch->getType()) {
            case ast_catch_clause: {
                operation_schema *catchScheme = new operation_schema(scheme_catch_clause);

                create_block(&current_context, normal_block);
                sharp_class *klass = compile_catch_clause(branch, catchScheme, tc_data, controlPaths);
                delete_block();

                if(klass != NULL && !catchedClasses.addif(klass)) {
                    create_new_error(GENERIC, ast->line, ast->col, "class `" + klass->fullName + "` has already been caught.");
                }

                if(klass != NULL && !is_class_related_to(klass, throwable)) {
                    create_new_error(GENERIC, ast->line, ast->col, "handling class `" + klass->fullName + "` must inherit base level exception class `std#throwable`");
                }


                create_jump_operation(catchScheme, endLabel);
                add_scheme_operation(subScheme, catchScheme);
                current_context.blockInfo.reachable = true;

                delete catchScheme;
                break;
            }

            case ast_finally_block: {
                operation_schema *finallyScheme = new operation_schema();
                create_try_catch_end_operation(finallyScheme, tc_data);

                sharp_label *nextFinallyLabel = retrieve_next_finally_label(&current_context.blockInfo);
                finally_data_info *finallyData = create_finally_data_tracker(tc_data, finallyScheme);

                create_set_label_operation(finallyScheme, endLabel);
                create_setup_local_field_operation(finallyScheme, exceptionObject);
                create_set_label_operation(finallyScheme, finallyBeginLabel);
                create_finally_start_operation(finallyScheme, finallyData);
                create_set_finally_field_operation(finallyData, exceptionObject, finallyScheme);

                compile_block(branch->getSubAst(ast_block), finallyScheme, finally_block);

                if(nextFinallyLabel == NULL) {
                    create_local_field_access_operation(finallyScheme, exceptionObject);
                    create_check_null_operation(finallyScheme);
                    create_jump_if_true_operation(finallyScheme, finallyEndLabel);

                    create_local_field_access_operation(finallyScheme, exceptionObject);
                    create_push_to_stack_operation(finallyScheme);
                    create_return_with_error_operation(finallyScheme); // it will only return of exception object still has not been handled

                    finallyScheme->schemeType =  scheme_last_finally_clause;
                } else {
                    create_local_field_access_operation(finallyScheme, exceptionObject);
                    create_check_null_operation(finallyScheme);
                    create_jump_if_true_operation(finallyScheme, finallyEndLabel);

                    create_jump_operation(finallyScheme, nextFinallyLabel); // it will only return of exception object still has not been handled

                    finallyScheme->schemeType = scheme_finally_clause;
                }

                create_set_label_operation(finallyScheme, finallyEndLabel);
                operation_schema *returnAddressScheme = new operation_schema();

                if(nextFinallyLabel == NULL) {
                    ALLOCATE_REGISTER_2X(0, 1, returnAddressScheme,
                         operation_schema *valueScheme = new operation_schema();
                         create_get_integer_constant_operation(valueScheme, 0);

                         create_get_value_operation(returnAddressScheme, valueScheme, false);
                         create_retain_numeric_value_operation(returnAddressScheme, register_0);

                         delete valueScheme; valueScheme = new operation_schema();
                         create_local_field_access_operation(valueScheme, returnAddressField);
                         create_get_value_operation(returnAddressScheme, valueScheme, false);
                         create_retain_numeric_value_operation(returnAddressScheme, register_1);

                         create_eq_eq_operation(returnAddressScheme, register_0, register_1);
                         create_jump_if_true_operation(returnAddressScheme, returnCheckEndLabel);
                         create_branch_operation(returnAddressScheme, register_1);

                         returnAddressScheme->schemeType = scheme_return_address_check_1;
                         delete valueScheme;
                    )

                    add_scheme_operation(finallyScheme, returnAddressScheme);
                } else {
                    ALLOCATE_REGISTER_2X(0, 1, returnAddressScheme,
                         operation_schema *valueScheme = new operation_schema();
                         create_get_integer_constant_operation(valueScheme, 0);

                         create_get_value_operation(returnAddressScheme, valueScheme, false);
                         create_retain_numeric_value_operation(returnAddressScheme, register_0);

                         delete valueScheme; valueScheme = new operation_schema();
                         create_local_field_access_operation(valueScheme, returnAddressField);
                         create_get_value_operation(returnAddressScheme, valueScheme, false);
                         create_retain_numeric_value_operation(returnAddressScheme, register_1);

                         create_eq_eq_operation(returnAddressScheme, register_0, register_1);
                         create_jump_if_false_operation(returnAddressScheme, nextFinallyLabel);

                         returnAddressScheme->schemeType = scheme_return_address_check_2;
                         delete valueScheme;
                    )

                    add_scheme_operation(finallyScheme, returnAddressScheme);
                }

                delete returnAddressScheme;
                create_set_label_operation(finallyScheme, returnCheckEndLabel);
                create_finally_end_operation(finallyScheme, finallyData);
                add_scheme_operation(subScheme, finallyScheme);

                delete finallyScheme;
                break;
            }

            default: break;
        }
    }

    if(!hasFinallyBlock) {
        create_set_label_operation(subScheme, endLabel);
        create_try_catch_end_operation(subScheme, tc_data);
    }

    subScheme->schemeType = scheme_try;
    catchedClasses.free();
    lockSchemes.free();
    add_scheme_operation(scheme, subScheme);
    delete subScheme;
}
