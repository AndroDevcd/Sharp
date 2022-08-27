//
// Created by BNunnally on 8/31/2021.
//

#include "class_preprocessor.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"
#include "../../settings/settings.h"
#include "../astparser/ast_parser.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"
#include "../types/sharp_field.h"
#include "../types/sharp_alias.h"
#include "alias_preprocessor.h"
#include "field_preprocessor.h"
#include "enum_preprocessor.h"
#include "generic_class_preprocessor.h"
#include "../types/sharp_function.h"

void pre_process() {
    sharp_file *file = currThread->currTask->file;
    file->errors = new ErrorManager(
            &file->tok->getLines(),
            file->name,
            true,
            options.aggressive_errors
    );

    sharp_class *globalClass = NULL;
    for(Int i = 0; i < file->p->size(); i++)
    {
        if(panic) return;

        Ast *trunk = file->p->astAt(i);
        if(i == 0) {
            if(trunk->getType() == ast_module_decl) {
                string package = concat_tokens(trunk);
                currModule = create_module(package);
                create_global_class();
            } else {
                string package = "__$srt_undefined";
                currModule = create_module(package);
                create_global_class();

                create_new_error(GENERIC, trunk->line, trunk->col, "module declaration must be "
                                        "first in every file");
            }

            file->module = currModule;
            globalClass = resolve_class(currModule, global_class_name, false, false);
            continue;
        }

        switch(trunk->getType()) {
            case ast_interface_decl:
            case ast_class_decl:
                pre_process_class(globalClass,NULL, trunk);
                break;
            case ast_alias_decl:
                pre_process_alias(globalClass, trunk);
                break;
            case ast_variable_decl:
                pre_process_field(globalClass, trunk);
                break;
            case ast_generic_class_decl:
            case ast_generic_interface_decl:
                pre_process_generic_class(globalClass, trunk);
                break;
            case ast_enum_decl:
                pre_process_enum(globalClass, NULL, trunk);
                break;
            case ast_module_decl: /* fail-safe */
                create_new_error(
                        GENERIC, trunk->line, trunk->col, "file module cannot be declared more than once");
                break;
            case ast_import_decl:
            case ast_delegate_decl:
            case ast_method_decl:
            case ast_mutate_decl:
            case ast_obfuscate_decl:
            case ast_component_decl:
                /* ignpre */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                create_new_error(
                        INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }
}


void pre_process_class(
        sharp_class *parentClass,
        sharp_class *with_class,
        Ast *ast) {

    Ast *block = ast->getLastSubAst();
    uInt flags = 0;
    string className = "";

    if(with_class == NULL) {
        if(ast->hasSubAst(ast_access_type)) {
            flags = parse_access_flags(
                    flag_public
                    | flag_private | flag_protected | flag_local
                    | flag_stable | flag_unstable | flag_extension,
                    "class", parentClass,
                    ast->getSubAst(ast_access_type)
            );

            if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
                && !check_flag(flags, flag_protected))
                flags |= flag_public;
        } else flags = flag_public;

        className = ast->getToken(0).getValue();
        check_decl_conflicts(ast, parentClass, "class", className);
        class_type ct = ast->getType() == ast_interface_decl ? class_interface : class_normal;
        if(check_flag(parentClass->flags, flag_global)) {
            with_class = create_class(currThread->currTask->file,
                    currModule, className, flags, ct, false, ast);
        } else {
            with_class = create_class(currThread->currTask->file,
                    parentClass, className, flags, ct, false, ast);
        }
    }

    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_interface_decl:
            case ast_class_decl:
                pre_process_class(with_class, NULL, trunk);
                break;
            case ast_variable_decl:
                pre_process_field(with_class, trunk);
                break;
            case ast_alias_decl:
                pre_process_alias(with_class, trunk);
                break;
            case ast_generic_class_decl:
            case ast_generic_interface_decl:
                pre_process_generic_class(with_class, trunk);
                break;
            case ast_enum_decl:
                pre_process_enum(with_class, NULL, trunk);
                break;
            case ast_mutate_decl:
            case ast_delegate_decl:
            case ast_method_decl:
            case ast_operator_decl:
            case ast_construct_decl:
            case ast_init_decl:
            case ast_init_func_decl:
            case ast_obfuscate_decl:
                /* ignore */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                create_new_error(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }
}

void check_decl_conflicts(Ast *ast, sharp_class *with_class, string type, string name) {
    void *data;
    List<sharp_function*> functions;
    if(type != "field" && (data = resolve_field(name, with_class)) != NULL) {
        auto field = (sharp_field*)data;
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with field `" + name + "`");
        print_impl_location(name, "field", field->implLocation);
    } else if(type != "alias" && (data = resolve_alias(name, with_class)) != NULL) {
        auto alias = (sharp_alias*)data;
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with alias `" + name + "`");
        print_impl_location(name, "alias", alias->location);
    } else if(type != "function" && locate_functions_with_name(name, with_class, undefined_function, false, false, functions)) {
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with function `" + name + "`");
        print_impl_location(name, "function", functions.first()->implLocation);
    } else if((type != "class" && type != "enum") && (data = resolve_class(with_class, name, false, false)) != NULL) {
        auto sc = (sharp_class*)data;
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with child class `" + name + "`");
        print_impl_location(name, "class", sc->implLocation);
    } else if((type != "class" && type != "enum") && (data = resolve_class(with_class, name, true, false)) != NULL) {
        auto sc = (sharp_class*)data;
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with child class `" + name + "`");
        print_impl_location(name, "class", sc->implLocation);
    } else if((type != "class" && type != "enum") && (data = resolve_class(currModule, name, false, false)) != NULL) {
        auto sc = (sharp_class*)data;
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with class `" + name + "`");
        print_impl_location(name, "class", sc->implLocation);
    } else if((type != "class" && type != "enum") && (data = resolve_class(currModule, name, true, false)) != NULL) {
        auto sc = (sharp_class*)data;
        create_new_warning(GENERIC, __w_decl, ast->line, ast->col,
                         "declared " + type + " `" + name + "` conflicts with class `" + name + "`");
        print_impl_location(name, "class", sc->implLocation);
    }
}
