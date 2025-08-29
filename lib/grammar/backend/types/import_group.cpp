//
// Created by BNunnally on 9/7/2021.
//
#include "import_group.h"
#include "../../sharp_file.h"
#include "../../compiler_info.h"

import_group* create_import_group(sharp_file *file, string name, Ast *ast) {
    impl_location location(file, 0, 0);
    import_group *ig = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((ig = resolve_import_group(file, name)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "import alias `" + name +
                                                                              "` is already defined in the file");
        print_impl_location(ig->name, "import alias", ig->implLocation);
        return ig;
    } else {
        ig = new import_group(
                name, location
        );

        GUARD(globalLock)
        file->importGroups.add(ig);
        return ig;
    }
}