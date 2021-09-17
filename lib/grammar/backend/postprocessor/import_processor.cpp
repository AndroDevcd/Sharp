//
// Created by BNunnally on 9/8/2021.
//

#include "import_processor.h"
#include "../../../../stdimports.h"
#include "../../sharp_file.h"
#include "../../taskdelegator/task_delegator.h"
#include "../astparser/ast_parser.h"
#include "../types/sharp_module.h"
#include "../types/sharp_class.h"
#include "../../settings/settings_processor.h"

void process_import(Ast *ast) {
    if(!options.magic) {
        List<sharp_module *> imports;

        for (Int i = 0; i < ast->getSubAstCount(); i++) {
            process_import_item(imports, ast->getSubAst(i));
        }

        if (ast->getTokenCount() > 0) {
            impl_location location(currThread->currTask->file,
                                   ast->line, ast->col);
            import_group *ig = new import_group(
                    ast->getToken(0).getValue(), location);

            ig->modules.addAll(imports);
            currThread->currTask->file->importGroups.add(ig);
        } else {
            currThread->currTask->file->imports.appendAllUnique(imports);
        }
    }
}

void process_import_item(List<sharp_module*> &imports, Ast *branch) {
    bool star = false;
    stringstream ss;
    for(long i = 0; i < branch->getTokenCount(); i++) {
        if(branch->getToken(i).getValue() != "*") {
            ss << branch->getToken(i).getValue();
        } else {
            star = true;
            break;
        }
    }

    string mod = ss.str();
    string baseMod = mod.substr(0, mod.size() - 1);
    sharp_module *package;

    if(star) {
        bool found = false;
        if((package = get_module(baseMod)) != NULL) {
            imports.addif(package);
        }

        for(long i = 0; i < modules.size(); i++) {
            if(startsWith(modules.get(i)->name, mod)) {
                found = true;
                imports.addif(modules.get(i));
            }
        }

        if(!found) {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, branch->line, branch->col, "modules under prefix `" + mod +
                                                                       "*` could not be found");
        }

    } else {
        if((package = get_module(mod)) != NULL) {
            if(!imports.addif(package)) {
                create_new_warning(GENERIC, __w_general, branch->line, branch->col, "module `" + mod + "` has already been imported.");
            }
        }
        else {
            currThread->currTask->file->errors->createNewError(
                    GENERIC, branch->line, branch->col, "module `" + mod + "` could not be found");
        }
    }
}

