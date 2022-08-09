//
// Created by bknun on 8/8/2022.
//

#include "field_generator.h"
#include "../../types/types.h"
#include "code/code_info.h"
#include "code/scheme/scheme_processor.h"

void generate_address(sharp_field *field) {
    bool isStatic = check_flag(field->flags, flag_static);
    Int address = 0;

    if(field->ci == NULL) {
        field->ci = new code_info();
    }

    for(Int i = 0; i < field->owner->fields.size(); i++) {
        sharp_field *sf = field->owner->fields.get(i);

        if(sf == field) {
            field->ci->address = address;
            break;
        } else if(isStatic == check_flag(sf->flags, flag_static) && sf->used) {
            address++;
        }
    }
}

code_info* get_or_initialize_code(sharp_field *field) {
    if(field->ci == NULL) {
        generate_address(field);
    }

    return field->ci;
}

void generate(sharp_field *field) {
    // todo: set container here when needed based off some logic
    process_scheme(field->scheme, get_or_initialize_code(field), NULL);

    if(field->name == "feep") {
        cout << "code:\n\n" << code_to_string(field->ci);
    }
}
