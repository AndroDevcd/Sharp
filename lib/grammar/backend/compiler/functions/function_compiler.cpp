//
// Created by bknun on 6/2/2022.
//

#include "function_compiler.h"
#include "../../context/context.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../types/types.h"
#include "../../../compiler_info.h"
#include "../compiler.h"
#include "statements/statement_compiler.h"
#include "../expressions/expression.h"
#include "../../../settings/settings.h"
#include "../../postprocessor/function_processor.h"
#include "../mutate_compiler.h"

void check_main_method_signature(sharp_function *function) {
    if(get_primary_class(&current_context) != NULL && check_flag(get_primary_class(&current_context)->flags, flag_global)
        && function->name == "main") {
        string std = "std";
        sharp_class *stringClass = resolve_class(get_module(std), "string", false, false);

        if(stringClass != NULL) {
            string args = "args";
            sharp_type stringType(stringClass, false, true);
            impl_location location;
            List<sharp_field*> params;
            uInt flags;
            params.add(new sharp_field (args, function->owner, location, stringType, flags, normal_field, function->ast));

            if(function_parameters_match(function->parameters, params, false, exclude_all)) {
                if(function->returnType == type_var) { // fn main(string[]) : var;
                    if(user_main_method == NULL) {
                        user_main_method = function;
                        main_method_sig = 0;
                    } else {
                        current_file->errors->createNewError(GENERIC, function->ast, "main method with the same or different signature already exists");
                        print_impl_location(function->name, "function", function->implLocation);
                    }

                } else if(function->returnType == type_nil) { // fn main(string[]);
                    if(user_main_method == NULL) {
                        user_main_method = function;
                        main_method_sig = 1;
                    } else {
                        current_file->errors->createNewError(GENERIC, function->ast, "main method with the same or different signature already exists");
                        print_impl_location(function->name, "function", function->implLocation);
                    }
                } else
                    create_new_warning(GENERIC, __w_main, function->ast->line, function->ast->col, "main method might not be executed");
            }

            deleteList(params);
            if(function_parameters_match(function->parameters, params, false, exclude_all)) {
                if(function->returnType == type_nil) { // fn main();
                    if(user_main_method == NULL) {
                        user_main_method = function;
                        main_method_sig = 2;
                    } else {
                        current_file->errors->createNewError(GENERIC, function->ast, "main method with the same or different signature already exists");
                        print_impl_location(function->name, "function", function->implLocation);
                    }
                } else if(function->returnType == type_var) { // fn main() : var;
                    if(user_main_method == NULL) {
                        user_main_method = function;
                        main_method_sig = 3;
                    } else {
                        current_file->errors->createNewError(GENERIC, function->ast, "main method with the same or different signature already exists");
                        print_impl_location(function->name, "function", function->implLocation);
                    }

                } else
                    create_new_warning(GENERIC, __w_main, function->ast->line, function->ast->col, "main method might not be executed");
            }
        } else
            current_file->errors->createNewError(GENERIC, function->ast, "class `std#string` was not found when analyzing main method");
    }
}

void call_base_constructor(Ast *ast, sharp_function *function) {
    if(ast->hasSubAst(ast_base_class_constructor)) {
        Ast *baseClassConstr = ast->getSubAst(ast_base_class_constructor);

        if(function->owner->baseClass != NULL) {
            List<expression*> expressions;
            List<sharp_field*> params;
            List<operation_schema*> paramOperations;

            string name = "";
            for(Int i = 0; i < baseClassConstr->getSubAst(ast_expression_list)->getSubAstCount(); i++) {
                expressions.add(new expression());
                compile_expression(*expressions.last(), baseClassConstr->getSubAst(ast_expression_list)->getSubAst(i));
                convert_expression_type_to_real_type(*expressions.last());

                impl_location location;
                params.add(new sharp_field(
                        name, get_primary_class(&currThread->currTask->file->context), location,
                        expressions.last()->type, flag_public, normal_field,
                        baseClassConstr->getSubAst(ast_expression_list)
                ));

                paramOperations.add(new operation_schema(expressions.last()->scheme));
            }

            sharp_function *constructor = resolve_function(function->owner->name, function->owner,
                         params,constructor_function, constructor_only, baseClassConstr, false, true);

            if(constructor != NULL) {
                compile_function_call(function->scheme, params, paramOperations, constructor, false, true,
                                      false);
            } else {
                current_file->errors->createNewError(GENERIC, baseClassConstr->line, baseClassConstr->col,
                                       " could not resolve base class constructor in class `" +
                                       function->owner->baseClass->name + "`.");
            }
        } else {
            current_file->errors->createNewError(GENERIC, baseClassConstr->line, baseClassConstr->col,
                                   " class `" +
                                   function->owner->fullName + "` does not inherit a base class.");
        }
    } else {
        if(function->owner->baseClass != NULL) {
            List<expression*> expressions;
            List<sharp_field*> params;
            List<operation_schema*> paramOperations;
            sharp_function *constructor = resolve_function(function->owner->name, function->owner,
                                                           params,constructor_function, constructor_only, function->ast, false, true);

            if(constructor != NULL) {
                compile_function_call(function->scheme, params, paramOperations, constructor, false, true, false);
            } else {
                current_file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col,
                                       " could not resolve base class constructor `" + function->owner->baseClass->name + "` in `" +
                                       function->owner->fullName + "`.");
            }
        }
    }
}

void compile_class_function(
        sharp_class *with_class,
        string name,
        function_type type,
        Ast *ast) {
    sharp_function *function = NULL;
    List<sharp_field*> params;
    process_function_parameters(params, with_class, ast->getSubAst(ast_utype_arg_list));

    function = resolve_function(name, with_class, params, type, exclude_all, ast, false, false);
    if(function != NULL) {
        compile_function(function, function->ast);
    } else {
        current_file->errors->createNewError(INTERNAL_ERROR, ast->line, ast->col,
                                             " could not resolve function `" + name + "` in `" + with_class->fullName + "`.");
    }
}

void compile_function(
        sharp_class *with_class,
        function_type type,
        Ast *ast) {
    string name;
    if(type == constructor_function) {
        name = ast->getToken(0).getValue();

        long pos;
        string class_name = with_class->name;
        if((pos = with_class->name.find('<')) != string::npos) {
            class_name = with_class->name.substr(0, pos);
        }

        if(name != class_name) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                               "constructor `" + name + "` must match holding class name `" + class_name + "`");
        }
    } else if(ast->hasSubAst(ast_refrence_pointer))
        process_extension_class(with_class, type == delegate_function, name, ast->getSubAst(ast_refrence_pointer));
    else
        name = ast->getToken(0).getValue();

    if(with_class != NULL) {
        if(with_class->blueprintClass) {
            GUARD(globalLock)

            for(Int i = 0; i < with_class->genericClones.size(); i++) {
                compile_class_function(with_class->genericClones.get(i), name, type, ast);
            }
        } else {
            compile_class_function(with_class, name, type, ast);
        }
    }
}

void compile_class_functions(sharp_class *with_class, Ast *block) {

    create_context(with_class, true);
    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_method_decl:
                compile_function(with_class, normal_function, trunk);
                break;
            case ast_operator_decl:
                compile_function(with_class, operator_function, trunk);
                break;
            case ast_construct_decl:
                compile_function(with_class, constructor_function, trunk);
                break;
            default:
                /* */
                break;
        }
    }

    delete_context();
}

void compile_class_mutations(sharp_class *with_class, Ast *block) {

    create_context(with_class, true);
    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_mutate_decl:
                compile_mutation(trunk);
                break;
            default:
                /* */
                break;
        }
    }

    delete_context();
}

void compile_function(sharp_function *function, Ast *ast) {
    create_context(&current_context, function, check_flag(function->flags, flag_static));
    if(function->scheme == NULL)
        function->scheme = new operation_schema(scheme_master);
    bool codePathsReturnValue;

    if(function->type == initializer_function) {
        codePathsReturnValue = compile_block(ast->getSubAst(ast_block), function->scheme);
    } else {
        check_main_method_signature(function);

        if((function->name == static_init_function_name || function->name == tls_init_function_name)
           && function->owner->fullName == platform_class_name) {
            if(ast->hasSubAst(ast_expression) || ast->getSubAst(ast_block)->sub_asts.size() > 0) {
                current_file->errors->createNewError(GENERIC, ast, "platform level function `" +
                        function_to_str(function) + "` does not allow statements or expressions inside.");
            }

            return;
        }

        if(ast->hasSubAst(ast_expression)) {
            codePathsReturnValue = true;
            create_block(&current_context, normal_block);
            current_context.blockInfo.line = ast->line;
            APPLY_TEMP_SCHEME(1, *current_context.functionCxt->scheme,
                  create_line_record_operation(&scheme_1, ast->line);
            )

            sharp_function *constructor;
            uInt match_result;
            expression e;
            compile_expression(e, ast->getSubAst(ast_expression));

            if(function->returnType != type_untyped && (match_result = is_implicit_type_match(function->returnType, e.type, constructor_only, constructor)) == no_match_found) {
                current_file->errors->createNewError(GENERIC, ast->line, ast->col, "return value of type `" +
                        type_to_str(e.type) + "` is not compatible with that of type `" + type_to_str(function->returnType) + "`");
            }

            if(match_result == match_constructor) {
                APPLY_TEMP_SCHEME(0, e.scheme,
                     compile_initialization_call(ast, constructor, e, &scheme_0);
                     e.scheme.free();
                )
            }

            if(function->returnType.type == type_nil) {
                function->scheme->copy(e.scheme);
                create_return_operation(function->scheme);
            } else if(is_numeric_type(e.type) && !get_real_type(e.type).isArray) {
                create_numeric_return_operation(function->scheme, &e.scheme);
            } else {
                create_object_return_operation(function->scheme, &e.scheme);
            }

            delete_block();
        } else {
            if(function->type == constructor_function) {
                create_block(&current_context, normal_block);
                call_base_constructor(ast, function);
                delete_block();
            }

            codePathsReturnValue = compile_block(ast->getSubAst(ast_block), function->scheme);

            if(function->returnType.type == type_nil) {
                create_return_operation(function->scheme);
            }
        }
    }


    if(!codePathsReturnValue && function->returnType.type != type_nil) {
        current_file->errors->createNewError(GENERIC, ast->getSubAst(ast_block), "not all code paths return a value");
    }
    delete_context();
}

bool compile_block(
        Ast *ast,
        operation_schema *scheme,
        block_type bt,
        sharp_label *beginLabel,
        sharp_label *endLabel,
        operation_schema *lockScheme,
        sharp_label *finallyLabel) {
    create_block(&current_context, bt);
    current_context.blockInfo.beginLabel = beginLabel;
    current_context.blockInfo.endLabel = endLabel;
    if(lockScheme)
        current_context.blockInfo.lockScheme = new operation_schema(*lockScheme);
    current_context.blockInfo.finallyLabel = finallyLabel;

    bool controlPaths[]
         = {
                 false, // MAIN_CONTROL_PATH
                 false, // IF_CONTROL_PATH
                 false, // ELSEIF_CONTROL_PATH
                 false,  // ELSE_CONTROL_PATH
                 false,  // TRY_CONTROL_PATH
                 false,  // CATCH_CONTROL_PATH
                 false,  // WHEN_CONTROL_PATH
                 false   // WHEN_ELSE_CONTROL_PATH
         };

    for(unsigned int i = 0; i < ast->getSubAstCount(); i++) {
        Ast *branch = ast->getSubAst(i);

        if(branch->getType() == ast_block) {
            compile_block(branch, scheme);
            continue;
        } else {
            branch = branch->getSubAst(0);
            compile_statement(branch, scheme, controlPaths);
        }
    }

    delete_block();
    return validate_control_paths(controlPaths);
}