//
// Created by bknun on 8/8/2022.
//

#include "file_generator.h"
#include "field_generator.h"

void generate(sharp_file *file) {

    // Generate code for all global static fields
    for(Int i = 0; i < file->fields.size(); i++) {
        sharp_field *sf = file->fields.get(i);

        if(sf->used) {
            generate(sf);
        }
    }
}
