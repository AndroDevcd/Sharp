//
// Created by bknun on 6/1/2022.
//

#include "init_compiler.h"
#include "../../../sharp_file.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../compiler_info.h"
#include "../../astparser/ast_parser.h"
#include "../../types/sharp_module.h"
#include "../../types/sharp_class.h"
#include "../../types/sharp_field.h"
#include "../../postprocessor/function_processor.h"
#include "function_compiler.h"


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
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                   "static initializer functions are not allowed.");
        }

        if(params.empty()) {
            currThread->currTask->file->errors->createNewError(GENERIC, ast,
                      "expected parameter for initializer function, but none were provided.");
        } else if(!params.singular()) {
            stringstream ss;
            ss << "expected 1 parameter for initializer function, but " << params.size() << " were provided.";
            currThread->currTask->file->errors->createNewError(GENERIC, ast, ss.str());
        }

        function = resolve_function(
                instance_init_name(with_class->name), with_class,
                params,initializer_function, exclude_all,
                ast, false, false
        );

        if(function == NULL) {
            currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, ast,
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

    compile_function(function);
}

void compile_inits(sharp_class* with_class) {
    Ast *block = with_class->ast->getSubAst(ast_block);

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
