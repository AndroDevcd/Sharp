//
// Created by BNunnally on 9/7/2021.
//

#include "enum_preprocessor.h"
#include "../../frontend/parser/Ast.h"
#include "../types/sharp_class.h"
#include "../astparser/ast_parser.h"
#include "../../taskdelegator/task_delegator.h"
#include "../types/sharp_field.h"
#include "class_preprocessor.h"
#include "../../compiler_info.h"

void pre_process_enum(
        sharp_class *parentClass,
        sharp_class *with_class,
        Ast *ast) {
    Ast* block = ast->getSubAst(ast_enum_identifier_list);
    uInt flags = 0;
    string className = "";

    if(with_class == NULL) {
        if(ast->hasSubAst(ast_access_type)) {
            flags = parse_access_flags(
                    flag_public
                    | flag_private | flag_protected | flag_local
                    | flag_stable | flag_unstable | flag_extension,
                    "enum", parentClass,
                    ast->getSubAst(ast_access_type)
            );

            if(!check_flag(flags, flag_public) && !check_flag(flags, flag_private)
               && !check_flag(flags, flag_protected))
                flags |= flag_public;
        } else flags = flag_public;

        flags |= flag_extension;
        className = ast->getToken(0).getValue();
        if(check_flag(parentClass->flags, flag_global)) {
            with_class = create_class(currThread->currTask->file,
                                      currModule, className, flags, class_enum, false, ast);
        } else {
            with_class = create_class(currThread->currTask->file,
                                      parentClass, className, flags, class_enum, false, ast);
        }

        check_decl_conflicts(ast, parentClass, "enum", className);
        for(Int i = 0; i < block->getSubAstCount(); i++) {
            Ast *trunk = block->getSubAst(i);

            switch(trunk->getType()) {
                case ast_class_decl: /* invalid */
                    break;
                case ast_variable_decl: /* invalid */
                    break;
                case ast_delegate_decl: /* invalid */
                case ast_method_decl: /* invalid */
                    break;
                case ast_operator_decl: /* invalid */
                    break;
                case ast_construct_decl: /* invalid */
                    break;
                case ast_interface_decl: /* invalid */
                    break;
                case ast_generic_class_decl: /* invalid */
                    break;
                case ast_generic_interface_decl: /* invalid */
                    break;
                case ast_enum_identifier:
                    pre_process_enum_field(with_class, trunk);
                    break;
                default:
                    stringstream err;
                    err << ": unknown ast type: " << trunk->getType();
                    currThread->currTask->file->errors->createNewError(INTERNAL_ERROR, trunk->line, trunk->col, err.str());
                    break;
            }
        }
    }
}

void pre_process_enum_field(sharp_class *parentClass, Ast *ast) {
    uInt flags = flag_public | flag_static | flag_const;
    string name = ast->getToken(0).getValue();

    create_field(
            currThread->currTask->file,
            parentClass, name, flags, sharp_type(parentClass),
            normal_field, ast
    );
}
