//
// Created by BNunnally on 8/31/2021.
//

#include "sharp_class.h"
#include "../dependency/dependancy.h"
#include "../../compiler_info.h"
#include "sharp_module.h"
#include "../../taskdelegator/task_delegator.h"


void create_global_class() {
    uInt flags;
    string name = global_class_name;
    set_flag(flags, flag_public, true);
    set_flag(flags, flag_global, true);
    sharp_class *gc;
    if((gc = resolve_class(currModule, name, false, false)) == NULL) {
        gc = create_class(currThread->currTask->file, currModule, name, flags, NULL);
        // TODO: add default construxtor
    }
}


sharp_class* create_class(
        sharp_file *file,
        sharp_module* module,
        string name,
        uInt flags,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_class *sc = NULL, *owner = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sc = resolve_class(module, name, false, false)) != NULL
        || (sc = resolve_class(module, name, true, false)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                  "` is already defined in module {" + module->name + "}");
        print_impl_location(sc, sc->implLocation);
        return sc;
    } else {
        owner = resolve_class(global_class_name, false, false);
        sc = new sharp_class(
                name, owner, module,
                location, flags, ast
        );

        GUARD(globalLock)
        if(owner != NULL) owner->children.add(sc);
        classes.add(sc);
        return sc;
    }

}