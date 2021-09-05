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
    }
}