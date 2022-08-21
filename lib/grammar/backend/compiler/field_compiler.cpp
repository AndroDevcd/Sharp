//
// Created by BNunnally on 10/8/2021.
//

#include "field_compiler.h"
#include "../../sharp_file.h"
#include "../astparser/ast_parser.h"
#include "../types/types.h"
#include "expressions/expression.h"
#include "../postprocessor/field_processor.h"
#include "../../compiler_info.h"
#include "../postprocessor/function_processor.h"
#include "functions/function_compiler.h"

void compile_class_fields(sharp_class *with_class, Ast *block) {

    create_context(with_class, true);
    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_variable_decl:
                compile_field(with_class, trunk);
                break;
            default:
                /* ignore */
                break;
        }
    }

    delete_context();
}

void compile_static_closure_reference(sharp_field *field) {
    GUARD2(globalLock)
    create_context(field);

    sharp_function *function;
    List<sharp_field*> params;
    sharp_type void_type(type_nil);
    string platform_kernel = "platform.kernel";
    sharp_class *with_class = resolve_class(get_module(platform_kernel), "platform", false, false);

    if(with_class != NULL) {
        function = resolve_function(
                static_init_function_name, with_class,
                params, normal_function, exclude_all,
                field->ast, false, false
        );

        if (function != NULL) {
            if(function->scheme == NULL)
                function->scheme = new operation_schema(scheme_master);

            APPLY_TEMP_SCHEME(0, (*function->scheme),
                 create_new_class_operation(&scheme_0, field->type._class);
                 create_static_field_access_operation(&scheme_0, field, false);
                 create_pop_value_from_stack_operation(&scheme_0);
            )
        } else {
            currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, field->ast,
                       " cannot locate internal function `static_init` in platform class for closure.");
        }
    } else {
        currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, field->ast,
                  " cannot locate platform class `platform` for closure.");
    }

    delete_context();
}

void inject_field_initialization(sharp_class *with_class, Ast *ast, sharp_field *field) {
    GUARD2(globalLock)
    string name;
    if(field->fieldType == normal_field && field->scheme) {
        sharp_function *function;
        List<sharp_field*> params;
        sharp_type void_type(type_nil);

        if(check_flag(field->flags, flag_static)) {
            function = resolve_function(
                    static_init_name(with_class->name), with_class,
                    params,normal_function, exclude_all,
                    ast, false, false
            );

            if(function == NULL) {
                name = static_init_name(with_class->name);
                create_function(
                        with_class, flag_private | flag_static,
                        normal_function, name,
                        false, params,
                        void_type, ast, function
                );

                function->scheme = new operation_schema(scheme_master);
            }
        } else {

            function = resolve_function(
                    instance_init_name(with_class->name), with_class,
                    params,normal_function, exclude_all,
                    ast, false, false
            );

            if(function == NULL) {
                name = instance_init_name(with_class->name);
                create_function(
                        with_class, flag_private,
                        normal_function, name,
                        false, params,
                        void_type, ast, function
                );

                function->scheme = new operation_schema(scheme_master);
            }
        }

        create_dependency(field);
        function->scheme->steps.add(new operation_step(operation_step_scheme, field->scheme));
    } else if(field->scheme) {
        sharp_function *function;
        List<sharp_field*> params;
        sharp_type void_type(type_nil);
        string platform_kernel = "platform.kernel";
        sharp_class *platformClass = resolve_class(get_module(platform_kernel), "platform", false, false);

        function = resolve_function(
                tls_init_function_name, platformClass,
                params,normal_function, exclude_all,
                ast, false, false
        );

        if(function->scheme == NULL) {
            function->scheme = new operation_schema(scheme_master);
        }

        create_dependency(field);
        function->scheme->steps.add(new operation_step(operation_step_scheme, field->scheme));
    }
}

// todo: come back later and support updating fields with closures attached to them as well
void compile_field(sharp_class *with_class, Ast *ast) {
    string name;
    if(parser::isStorageType(ast->getToken(0))) {
        name = ast->getToken(1).getValue();
    } else name = ast->getToken(0).getValue();

    sharp_field *field = resolve_field(name, with_class);

    compile_field(field, ast);

    if(field->getter) {
        compile_function(field->getter, field->getter->ast);
    }

    if(field->setter) {
        field->setter->parameters.get(0)->type.copy(field->type);
        compile_function(field->setter, field->setter->ast);
    }

    inject_field_initialization(with_class, ast, field);
    if(ast->hasSubAst(ast_variable_decl)) {
        long startAst = 0;
        for (long i = 0; i < ast->getSubAstCount(); i++) {
            if (ast->getSubAst(i)->getType() == ast_variable_decl) {
                startAst = i;
                break;
            }
        }

        for (long i = startAst; i < ast->getSubAstCount(); i++) {
            Ast *trunk = ast->getSubAst(i);

            name = trunk->getToken(0).getValue();
            sharp_field *xtraField = resolve_field(name, field->owner);

            if (xtraField && xtraField->scheme != NULL && xtraField->scheme->schemeType != scheme_none) {
                inject_field_initialization(with_class, ast, xtraField);
            }
        }
    }
}

void compile_field_injection_request(sharp_field *field, Ast *ast) {
    get_component_request *request = field->request->to_component_request();
    type_definition *td = get_type_definition(componentManager, field->type, *request);
    delete request; request = NULL;

    if(td) {
        field->scheme->copy(*td->scheme);
    } else {
        currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                                           "cannot inject field `" + field->name + ": " +
                                                           type_to_str(field->type) + "`, no type found in components.");
    }
}

void compile_field(sharp_field *field, Ast *ast) {
    GUARD(globalLock)
    create_context(field);

    if(field->name == "foo" && field->owner->name == "_object_") {
        // todo: remove here for testing only
        string fname = "%test";
        List<sharp_field*> params;
        sharp_type returnType(type_nil);
        create_function(field->owner, flag_public, normal_function, fname, false,
                params, returnType, ast);

        create_block(&current_context, normal_block);
        sharp_function *fun = field->owner->functions.last();
        create_context(fun);
        expression e;
        compile_expression(e, ast->getSubAst(ast_expression));

        delete_block();
        validate_function_type(false, fun, e.type, &e.scheme, fun->ast);
        delete_context();
    }

    if(field->request != NULL) {
        compile_field_injection_request(field, ast);
    }

    if(field->scheme == NULL) {

        if(ast->hasSubAst(ast_expression)) {
            expression e;
            field->scheme = new operation_schema();
            create_context(field->owner, check_flag(field->flags, flag_static));
            compile_expression(e, ast->getSubAst(ast_expression));
            delete_context();

            if(field->type == type_untyped) {
                validate_field_type(false, field, e.type, &e.scheme,
                                    ast->getSubAst(ast_expression));
            } else {
                convert_expression_type_to_real_type(e);
                if(!is_implicit_type_match(
                        field->type, e.type, match_operator_overload | match_constructor)) {
                    currThread->currTask->file->errors->createNewError(GENERIC, ast,
                               "cannot assign field `" + field->name + ": " +
                                    type_to_str(field->type) + "` type `" + type_to_str(e.type) + "`, as types do not match.");
                }
            }
            field->scheme->copy(e.scheme);
        }

        if(ast->hasSubAst(ast_getter) && field->getter == NULL) {
            process_getter(field, ast->getSubAst(ast_getter));
        }

        if(ast->hasSubAst(ast_setter) && field->setter == NULL) {
            process_setter(field, ast->getSubAst(ast_setter));
        }

        delete_context();
        if(ast->hasSubAst(ast_variable_decl)) {
            long startAst = 0;
            for(long i = 0; i < ast->getSubAstCount(); i++) {
                if(ast->getSubAst(i)->getType() == ast_variable_decl)
                {
                    startAst = i;
                    break;
                }
            }

            for(long i = startAst; i < ast->getSubAstCount(); i++) {
                Ast *trunk = ast->getSubAst(i);

                string name = trunk->getToken(0).getValue();
                sharp_field *xtraField = resolve_field(name, field->owner);

                if(xtraField) {
                    create_context(xtraField);
                    xtraField->fieldType = field->fieldType;
                    xtraField->type.copy(field->type);

                    if(trunk->hasSubAst(ast_expression)) {
                        expression e;
                        xtraField->scheme = new operation_schema();
                        compile_expression(e, trunk->getSubAst(ast_expression));

                        if(xtraField->type == type_untyped) {
                            validate_field_type(false, xtraField, e.type, &e.scheme,
                                                ast->getSubAst(ast_expression));
                        } else {
                            convert_expression_type_to_real_type(e);
                            if(!is_implicit_type_match(
                                    xtraField->type, e.type, match_operator_overload | match_constructor)) {
                                currThread->currTask->file->errors->createNewError(GENERIC, ast,
                                            "cannot assign field `" + xtraField->name + ": " +
                                                  type_to_str(xtraField->type) + "` type `" + type_to_str(e.type) + "`, as types do not match.");
                            }
                        }

                        xtraField->scheme->copy(e.scheme);
                    }

                    delete_context();
                }
            }
        }
    }
}