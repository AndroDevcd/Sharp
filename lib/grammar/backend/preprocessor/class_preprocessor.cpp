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

void pre_process_class() {
    sharp_file *file = currThread->currTask->file;
    file->errors = new ErrorManager(
            &file->tok->getLines(),
            file->name,
            true,
            options.aggressive_errors
    );

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

                currThread->currTask->file->errors->createNewError(GENERIC, trunk->line, trunk->col, "module declaration must be "
                                                                           "first in every file");
            }

            continue;
        }

        switch(trunk->getType()) {
            case ast_class_decl:
                pre_process_class(
                        resolve_class(currModule, global_class_name, false, false),
                        NULL, trunk
                );
                break;
            case ast_module_decl: /* fail-safe */
                currThread->currTask->file->errors->createNewError(
                        GENERIC, trunk->line, trunk->col, "file module cannot be declared more than once");
                break;
            case ast_import_decl:
            case ast_interface_decl:
            case ast_generic_class_decl:
            case ast_generic_interface_decl:
            case ast_enum_decl:
            case ast_delegate_decl:
            case ast_method_decl:
            case ast_variable_decl:
            case ast_alias_decl:
            case ast_mutate_decl:
            case ast_obfuscate_decl:
                /* ignpre */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                currThread->currTask->file->errors->createNewError(
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

    if(with_class != NULL) {
        if(ast->hasSubAst(ast_access_type)) {
            flags = parse_access_flags(
                    flag_public
                    | flag_private | flag_protected | flag_local
                    | flag_stable | flag_unstable | flag_extension,
                    "class", parentClass, block);

            if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
                && !check_flag(flags, flag_protected))
                flags |= flag_public;
        } else flags = flag_public;

        className = ast->getToken(0).getValue();
        if(check_flag(parentClass->flags, flag_global)) {
            with_class = create_class(currThread->currTask->file,
                    currModule, className, flags, class_normal, ast);
        } else {
            with_class = create_class(currThread->currTask->file,
                    parentClass, className, flags, class_normal, ast);
        }
    }

    for(Int i = 0; i < block->getSubAstCount(); i++) {
        Ast *trunk = block->getSubAst(i);

        switch(trunk->getType()) {
            case ast_class_decl:
                pre_process_class(with_class, NULL, trunk);
                break;
            case ast_variable_decl:
                /* todo */
                break;
            case ast_alias_decl:
                /* todo */
                break;
            case ast_interface_decl:
                /* todo */
                break;
            case ast_generic_class_decl:
                /* todo */
                break;
            case ast_generic_interface_decl:
                /* todo */
                break;
            case ast_enum_decl:
                /* todo */
                break;
            case ast_mutate_decl:
                /* todo */
                break;
            case ast_delegate_decl:
            case ast_method_decl:
            case ast_operator_decl:
            case ast_construct_decl:
            case ast_init_decl:
            case ast_obfuscate_decl:
                /* ignore */
                break;
            default:
                stringstream err;
                err << ": unknown ast type: " << trunk->getType();
                currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                break;
        }
    }
}
