//
// Created by bknun on 6/1/2022.
//

#include "init_compiler.h"
#include "../../../sharp_file.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../compiler_info.h"
#include "../../astparser/ast_parser.h"
#include "../../types/types.h"
#include "../../postprocessor/function_processor.h"
#include "function_compiler.h"
#include "../expressions/expression.h"


void compile_init_declaration(sharp_class* with_class, Ast *ast) {
    uInt flags = flag_none;
    List<sharp_field*> params;
    sharp_function *function = NULL;
    if(ast->hasSubAst(ast_access_type)) {
        flags = parse_access_flags(
                flag_static,
                "init", with_class,
                ast->getSubAst(ast_access_type)
        );

    }

    if(ast->getType() == ast_init_func_decl) {
        process_function_parameters(params, with_class, ast->getSubAst(ast_utype_arg_list));

        if(check_flag(flags, flag_static)) {
            create_new_error(GENERIC, ast,
                   "static initializer functions are not allowed.");
        }

        if(params.empty()) {
            create_new_error(GENERIC, ast,
                      "expected parameter for initializer function, but none were provided.");
        } else if(!params.singular()) {
            stringstream ss;
            ss << "expected 1 parameter for initializer function, but " << params.size() << " were provided.";
            create_new_error(GENERIC, ast, ss.str());
        }

        function = resolve_function(
                instance_init_name(with_class->name), with_class,
                params,initializer_function, exclude_all,
                ast, false, false
        );

        if(function == NULL) {
            create_new_error(INTERNAL_ERROR, ast,
                         "could not locate initializer function in class `" + with_class->fullName + "`.");
        }
    }

    if(function == NULL) {
        sharp_type void_type(type_nil);
        if (check_flag(flags, flag_static)) { // here we assume that `params` must be empty if function == NULL
            function = resolve_function(
                    static_init_name(with_class->name), with_class,
                    params,normal_function, exclude_all,
                    ast, false, false
            );

            if(function == NULL) {
                string name = static_init_name(with_class->name);
                create_function(
                        with_class, flag_private | flag_static,
                        normal_function, name,
                        false, params,
                        void_type, ast, function
                );

                compile_static_initialization_check(function);
            }
        } else {
            function = resolve_function(
                    instance_init_name(with_class->name), with_class,
                    params,normal_function, exclude_all,
                    ast, false, false
            );

            if(function == NULL) {
                string name = instance_init_name(with_class->name);
                create_function(
                        with_class, flag_private,
                        normal_function, name,
                        false, params,
                        void_type, ast, function
                );
            }
        }

        for(Int i = 0; i < function->parameters.size(); i++) {
            function->parameters.get(i)->block = 0;
        }

        function->locals.addAll(function->parameters);
    }

    compile_function(function, ast);
}

void compile_inits(sharp_class* with_class, Ast *block) {

    for (Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch (trunk->getType()) {
            case ast_init_decl:
            case ast_init_func_decl:
                compile_init_declaration(with_class, trunk);
                break;
            default:
                break;
        }
    }
}

void compile_initialization_paring(sharp_function *constructor) {
    sharp_function *function;
    List<sharp_field*> params;
    List<operation_schema*> paramOperations;
    sharp_type void_type(type_nil);
    string name = "";

    GUARD2(globalLock)
    function = resolve_function(
            instance_init_name(constructor->owner->name), constructor->owner,
            params,normal_function, exclude_all,
            constructor->owner->ast, false, false
    );

    if(function == NULL) {
        name = instance_init_name(constructor->owner->name);
        create_function(
                constructor->owner, flag_private,
                normal_function, name,
                false, params,
                void_type, constructor->owner->ast, function
        );
    }

    if(constructor->type == constructor_function) {
        if(constructor->scheme == NULL)
            constructor->scheme = new operation_schema(scheme_master);

        APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
            create_context(constructor);
            compile_function_call(&scheme_0, params, paramOperations, function, false, true, false);
            create_unused_data_operation(&scheme_0);
            delete_context();
        )

        constructor->scheme->steps.add(new operation_step(operation_step_scheme, &scheme_0));
    }
}

void compile_static_initialization_requirement(sharp_function *constructor) {
    sharp_function *function;
    List<sharp_field*> params;
    sharp_type void_type(type_nil);
    string name = "";

    GUARD2(globalLock)
    function = resolve_function(
            static_init_name(constructor->owner->name), constructor->owner,
            params,normal_function, exclude_all,
            constructor->owner->ast, false, false
    );

    if(function == NULL) {
        name = static_init_name(constructor->owner->name);
        create_function(
                constructor->owner, flag_private | flag_static,
                normal_function, name,
                false, params,
                void_type, constructor->owner->ast, function
        );

        compile_static_initialization_check(function);
    }

    if(constructor->type == constructor_function) {
        if(constructor->scheme == NULL)
            constructor->scheme = new operation_schema(scheme_master);

        APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
            APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
                auto *staticInitFlag = resolve_field(static_init_flag_name, constructor->owner, false);
                create_static_field_access_operation(&scheme_0, staticInitFlag);
                create_dependency(constructor, staticInitFlag);
                create_dependency(function);
            )

            stringstream ss;
            List<operation_schema*> emptyParams;
            set_internal_label_name(ss, "if_end", uniqueId++)
            auto endLabel = create_label(ss.str(), &current_context, constructor->ast, &scheme_1);
            create_get_value_operation(&scheme_1, &scheme_0, false);
            create_jump_if_true_operation(&scheme_1, endLabel);
            create_static_function_call_operation(&scheme_1, emptyParams, function, false);
            create_set_label_operation(&scheme_1, endLabel);
        )

        constructor->scheme->steps.add(new operation_step(operation_step_scheme, &scheme_1));
    }
}

void compile_static_initialization_check(sharp_function *initFunc) {
    auto *staticInitFlag = resolve_field(static_init_flag_name, initFunc->owner, false);

    if(staticInitFlag != NULL) {
        if (initFunc->scheme == NULL)
            initFunc->scheme = new operation_schema(scheme_master);

        create_context(initFunc);
        APPLY_TEMP_SCHEME_WITHOUT_INJECT(1,
            APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
               create_static_field_access_operation(&scheme_0, staticInitFlag);
               create_dependency(initFunc, staticInitFlag);
            )

            stringstream ss;
            List<operation_schema*> emptyParams;
            set_internal_label_name(ss, "if_end", uniqueId++)
            auto endLabel = create_label(ss.str(), &current_context, initFunc->ast, &scheme_1);
            create_get_value_operation(&scheme_1, &scheme_0, false);
            create_jump_if_false_operation(&scheme_1, endLabel);
            create_return_operation(&scheme_1);
            create_set_label_operation(&scheme_1, endLabel);

            APPLY_TEMP_SCHEME_WITHOUT_INJECT(4,
                APPLY_TEMP_SCHEME_WITHOUT_INJECT(2,
                    create_get_integer_constant_operation(&scheme_2, 1);
                )
                create_value_assignment_operation(&scheme_4, &scheme_0, &scheme_2);
            )

            add_scheme_operation(&scheme_1, &scheme_4);
            create_unused_data_operation(&scheme_1);
        )

        delete_context();

        initFunc->scheme->steps.add(new operation_step(operation_step_scheme, &scheme_1));
    }
}
