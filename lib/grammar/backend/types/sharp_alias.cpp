//
// Created by BNunnally on 9/6/2021.
//

#include "sharp_alias.h"
#include "sharp_class.h"
#include "../../compiler_info.h"

void set_full_name(sharp_alias *alias) {
    alias->fullName = alias->owner->fullName + "." + alias->name;
}
;

sharp_alias* create_alias(
        sharp_file *file,
        sharp_class *owner,
        string name,
        uInt flags,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_alias *sa = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sa = resolve_alias(name, file)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                  "` is already defined in class: " + sa->owner->fullName);
        print_impl_location(sa->name, "alias", sa->location);
        return sa;
    } else {
        // todo: check for fields with similar name as alias and block it
        sa = new sharp_alias(name, owner, flags, location);

        GUARD(owner->mut)
        owner->aliases.add(sa);
        return sa;
    }
}

sharp_alias* create_alias(
        sharp_file *file,
        sharp_module *module,
        string name,
        uInt flags,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_alias *sa = NULL;
    sharp_class *owner = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }
    if((sa = resolve_alias(name, module)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                                                                              "` is already defined in module {" + sa->owner->module->name + "}");
        print_impl_location(sa->name, "alias", sa->location);
        return sa;
    } else {
        owner = resolve_class(module, global_class_name, false, false);
        sa = new sharp_alias(name, owner, flags, location);

        GUARD(owner->mut)
        owner->aliases.add(sa);
        return sa;
    }
}

void sharp_alias::free() {
    dependencies.free();
}
