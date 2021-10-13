//
// Created by BNunnally on 9/8/2021.
//

#include "generic_class_preprocessor.h"
#include "../../../../stdimports.h"
#include "../types/sharp_class.h"
#include "../../frontend/parser/Ast.h"
#include "../astparser/ast_parser.h"
#include "../../taskdelegator/task_delegator.h"
#include "class_preprocessor.h"
#include "../../compiler_info.h"

void pre_process_generic_class(sharp_class *parentClass, Ast *ast) {
    uInt flags = 0;
    string className = "";
    sharp_class *sc = NULL;

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
    class_type ct = ast->getType() == ast_generic_interface_decl ? class_interface : class_normal;
    if(check_flag(parentClass->flags, flag_global)) {
        sc = create_class(currThread->currTask->file,
                     currModule, className, flags, ct, true, ast);
    } else {
        sc = create_class(currThread->currTask->file,
                     parentClass, className, flags, ct, true, ast);
    }

    sc->blueprintClass = true;
    check_decl_conflicts(ast, parentClass, "class", className);
    if(sc->genericTypes.empty()) {
        // generic class is new
        pre_process_generic_identifier_list(sc, ast->getSubAst(ast_generic_identifier_list));

        stringstream ss;

        ss << "<";
        for(Int i = 0; i < sc->genericTypes.size(); i++) {
            ss << sc->genericTypes.get(i).name;

            if ((i + 1) < sc->genericTypes.size()) {
                ss << ", ";
            }
        }
        ss << ">";
        sc->fullName += ss.str();
    }
}
generic_type_identifier pre_process_generic_identifier(Ast *ast) {
    generic_type_identifier gt;

    gt.name = ast->getToken(0).getValue();
    return gt;
}

void pre_process_generic_identifier_list(sharp_class *genericClass, Ast *ast) {

    for(Int i = 0; i < ast->getSubAstCount(); i++) {
        generic_type_identifier gt = pre_process_generic_identifier(ast->getSubAst(i));

        if(locate_generic_type(gt.name, genericClass) == NULL)
            genericClass->genericTypes.add(gt);
        else {
            stringstream err;
            err << "duplicate generic key `" << gt.name << "`";
            currThread->currTask->file->errors->createNewError(GENERIC, ast->line, ast->col, err.str());
        }
    }
}