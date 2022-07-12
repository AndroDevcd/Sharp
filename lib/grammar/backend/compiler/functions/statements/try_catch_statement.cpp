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
    catch_data *catchData = create_catch_data_tracker(tc_data, scheme);
    sharp_class *exceptionClass = NULL;

    sharp_type handlingClass = resolve(ast->getSubAst(ast_utype_arg)->getSubAst(ast_utype));
    if(ast->getSubAst(ast_utype_arg)->getTokenCount() > 0) {
        sharp_field *catchField;
        uInt flags = flag_public;
        string name = ast->getSubAst(ast_utype_arg)->getToken(0).getValue();

        if(handlingClass.isArray) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` cannot evaluate to an array");
        }

        if(get_class_type(handlingClass) == NULL) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "type assigned to field `" + name + "` must be of type class: `" +
                    type_to_str(handlingClass) + "`");
        } else {
            create_set_catch_class_operation(catchData, get_class_type(handlingClass), scheme);
        }

        catchField = create_local_field(current_file, &current_context, name, flags, handlingClass, normal_field, ast);
        create_set_catch_field_operation(catchData, catchField, scheme);
    } else {

        if(handlingClass.isArray) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "handling class  `" +  type_to_str(handlingClass) + "` cannot evaluate to an array");
        }

        if(get_class_type(handlingClass) == NULL) {
            current_file->errors->createNewError(GENERIC, ast->line, ast->col, "type `" +  type_to_str(handlingClass) + "` must be of type class");
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
    List<sharp_class*> catchedClasses;
    List<operation_schema*> lockSchemes;
    sharp_label *finallyBeginLabel, *finallyEndLabel, *endLabel;
    operation_schema *subScheme = new operation_schema();
    subScheme->schemeType = scheme_try;

    set_internal_label_name(ss, "finally_begin", uniqueId++)
    finallyBeginLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "finally_end", uniqueId++)
    finallyEndLabel = create_label(ss.str(), &current_context, ast, subScheme);
    set_internal_label_name(ss, "try_end", uniqueId++)
    endLabel = create_label(ss.str(), &current_context, ast, subScheme);

    sharp_class *throwable = resolve_class(get_module(std), "throwable", false, false);
    bool hasFinallyBlock = ast->hasSubAst(ast_finally_block);

    if(hasFinallyBlock) {
        uInt flags = flag_public;
        set_internal_label_name(ss, "exception_object", 0)
        if((exceptionObject = resolve_local_field(ss.str(), &current_context)) == NULL) {
            sharp_type fieldType = sharp_type(type_object);
            exceptionObject =  create_local_field(current_file, &current_context, ss.str(), flags, fieldType, normal_field, ast);
        }
    }

    tc_data = create_try_catch_data_tracker(subScheme);
    create_try_catch_start_operation(subScheme, tc_data);
    create_try_catch_block_start_operation(subScheme, tc_data);

    controlPaths[TRY_CONTROL_PATH] = compile_block(ast->getSubAst(ast_block), subScheme, try_block, NULL, NULL, NULL, finallyBeginLabel);
    create_try_catch_block_end_operation(subScheme, tc_data);

    create_jump_operation(subScheme, endLabel);

    controlPaths[CATCH_CONTROL_PATH] = true;
    for(Int i = 1; i < ast->getSubAstCount(); i++) {
        Ast *branch = ast->getSubAst(i);

        current_context.blockInfo.reachable = true;
        switch (branch->getType()) {
            case ast_catch_clause: {
                sharp_class *klass = compile_catch_clause(branch, subScheme, tc_data, controlPaths);

                if(klass != NULL && !catchedClasses.addif(klass)) {
                    current_file->errors->createNewError(GENERIC, ast->line, ast->col, "class `" + klass->fullName + "` has already been caught.");
                }

                if(klass != NULL && !is_class_related_to(klass, throwable)) {
                    current_file->errors->createNewError(GENERIC, ast->line, ast->col, "handling class `" + klass->fullName + "` must inherit base level exception class `std#throwable`");
                }


                create_jump_operation(subScheme, endLabel);
                current_context.blockInfo.reachable = true;
                break;
            }

            case ast_finally_block: {
                create_try_catch_end_operation(subScheme, tc_data);

                sharp_label *nextFinallyLabel = retrieve_next_finally_label(&current_context.blockInfo);
                finally_data *finallyData = create_finally_data_tracker(tc_data, subScheme);

                create_set_label_operation(subScheme, endLabel);
                create_setup_local_field_operation(subScheme, exceptionObject);
                create_set_label_operation(subScheme, finallyBeginLabel);
                create_finally_start_operation(subScheme, finallyData);
                create_set_finally_field_operation(finallyData, exceptionObject, subScheme);

                compile_block(branch->getSubAst(ast_block), subScheme, finally_block);

                if(nextFinallyLabel == NULL) {
                    create_local_field_access_operation(subScheme, exceptionObject);
                    create_check_null_operation(subScheme);
                    create_jump_if_true_operation(subScheme, finallyEndLabel);


                    retrieve_lock_schemes(&current_context.blockInfo, lockSchemes);
                    for(Int j = 0; j < lockSchemes.size(); j++) { // todo look into also processing finally blocks in the same way
                        operation_schema* tmp = new operation_schema();
                        create_unlock_operation(tmp, lockSchemes.get(j));
                        add_scheme_operation(subScheme, tmp);
                    }

                    create_local_field_access_operation(subScheme, exceptionObject);
                    create_push_to_stack_operation(subScheme);
                    create_return_with_error_operation(subScheme); // it will only return of exception object still has not been handled
                } else {
                    create_local_field_access_operation(subScheme, exceptionObject);
                    create_check_null_operation(subScheme);
                    create_jump_if_true_operation(subScheme, finallyEndLabel);

                    create_jump_operation(subScheme, nextFinallyLabel); // it will only return of exception object still has not been handled

                }

                create_set_label_operation(subScheme, finallyEndLabel);
                create_finally_end_operation(subScheme, finallyData);
                break;
            }

            default: break;
        }
    }

    if(!hasFinallyBlock) {
        create_set_label_operation(subScheme, endLabel);
        create_try_catch_end_operation(subScheme, tc_data);
    }
    create_no_operation(subScheme);

    catchedClasses.free();
    lockSchemes.free();
    add_scheme_operation(scheme, subScheme);
}
