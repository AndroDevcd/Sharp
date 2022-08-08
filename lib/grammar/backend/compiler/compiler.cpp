//
// Created by bknun on 6/2/2022.
//

#include "compiler.h"
#include "../../sharp_file.h"
#include "../../compiler_info.h"
#include "../astparser/ast_parser.h"
#include "../types/types.h"
#include "field_compiler.h"
#include "functions/init_compiler.h"
#include "functions/function_compiler.h"
#include "mutate_compiler.h"
#include "enum_compiler.h"
#include "../../settings/settings.h"

void compile_static_closure_references(sharp_class *with_class) {
    for(Int i = 0; i < with_class->fields.size(); i++) {
        if(with_class->fields.get(i)->staticClosure) {
            compile_static_closure_reference(with_class->fields.get(i));
        }
    }
}

void compile_global_members() {
    sharp_file *file = current_file;
    sharp_class *globalClass = NULL;

    for(Int i = 0; i < file->p->size(); i++)
    {
        if(panic) return;

        Ast *trunk = file->p->astAt(i);
        if(i == 0) {
            if(trunk->getType() == ast_module_decl) {
                string package = concat_tokens(trunk);
                currModule = create_module(package);
            } else {
                string package = "__$srt_undefined";
                currModule = create_module(package);
            }

            globalClass = resolve_class(currModule, global_class_name, false, false);
            create_context(globalClass, true);
            continue;
        }

        switch(trunk->getType()) {
            case ast_variable_decl:
                compile_field(globalClass, trunk);
                break;
            case ast_method_decl:
                compile_function(globalClass, normal_function, trunk);
                break;
            case ast_operator_decl:
                compile_function(globalClass, operator_function, trunk);
                break;
            default:
                /* */
                break;
        }
    }

    compile_static_closure_references(globalClass);
    delete_context();
}


/**
 * This function ensures that all control paths return a value before leaving a block.
 * @param controlPaths
 * @return
 */
bool validate_control_paths(bool *controlPaths) {
    return controlPaths[MAIN_CONTROL_PATH]
           || (controlPaths[IF_CONTROL_PATH] && controlPaths[ELSEIF_CONTROL_PATH] && controlPaths[ELSE_CONTROL_PATH])
           || (controlPaths[TRY_CONTROL_PATH] && controlPaths[CATCH_CONTROL_PATH])
           || (controlPaths[WHEN_CONTROL_PATH] && controlPaths[WHEN_ELSE_CONTROL_PATH]);
}
