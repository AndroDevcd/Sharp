//
// Created by BNunnally on 9/1/2021.
//

#include "sharp_field.h"
#include "../types/sharp_module.h"
#include "../../compiler_info.h"
#include "../types/sharp_class.h"

sharp_field* create_field(
        sharp_file *file,
        sharp_module *module,
        string name,
        uInt flags,
        sharp_type type,
        field_type ft,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_field *sf = NULL;
    sharp_class *owner = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sf = resolve_field(name, module)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                             "` is already defined in module {" + module->name + "}");
        print_impl_location(sf->name, "field", sf->implLocation);
        return sf;
    } else {
        owner = resolve_class(global_class_name, false, false);
        sf = new sharp_field(
                name, owner, location,
                type, flags, ft, ast
        );

        GUARD(owner->mut)
        owner->fields.add(sf);
        return sf;
    }
}

sharp_field* create_field(
        sharp_file *file,
        sharp_class *sc,
        string name,
        uInt flags,
        sharp_type type,
        field_type ft,
        Ast *ast) {
    impl_location location(file, 0, 0);
    sharp_field *sf = NULL;

    if(ast != NULL) {
        location.line = ast->line;
        location.col = ast->col;
    }

    if((sf = resolve_field(name, sc)) != NULL) {
        file->errors->createNewError(PREVIOUSLY_DEFINED, ast->line, ast->col, "class `" + name +
                             "` is already defined in class: " + sc->name);
        print_impl_location(sf->name, "field", sf->implLocation);
        return sf;
    } else {
        sf = new sharp_field(
                name, sc, location,
                type, flags, ft, ast
        );

        GUARD(sc->mut)
        sc->fields.add(sf);
        return sf;
    }
}

void sharp_field::set_full_name() {
    fullName = owner->fullName + "." + name;
}
