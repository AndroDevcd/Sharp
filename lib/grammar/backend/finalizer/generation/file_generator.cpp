//
// Created by bknun on 8/8/2022.
//

#include "file_generator.h"
#include "field_generator.h"
#include "class_generator.h"
#include "function_generator.h"
#include "../../types/types.h"

void generate_addresses(sharp_file *file) {
    // Static Fields
    for(Int i = 0; i < file->fields.size(); i++) {
        sharp_field *sf = file->fields.get(i);

        if(sf->used) {
            generate_address(sf);
        }
    }

    // classes
    for(Int i = 0; i < file->classes.size(); i++) {
        sharp_class *sc = file->classes.get(i);

        if(sc->used) {
            generate_class_addresses(sc);
        }
    }

    // Static Functions
    for(Int i = 0; i < file->functions.size(); i++) {
        sharp_function *sf = file->functions.get(i);

        if(sf->used) {
            generate_address(sf);
        }
    }
}
