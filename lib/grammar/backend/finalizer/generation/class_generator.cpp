//
// Created by bknun on 8/8/2022.
//

#include "class_generator.h"
#include "../../types/types.h"
#include "field_generator.h"
#include "function_generator.h"
#include "generator.h"

uInt classAddressCounter = 0;
void generate_address(sharp_class *sc) {
    if(sc->ci == NULL) {
        sc->ci = new code_info();
        sc->ci->uuid = UUIDGenerator++;
        sc->ci->address = classAddressCounter++;
        compressedCompilationClasses.addif(sc);
    }
}

code_info* get_or_initialize_code(sharp_class *sc) {
    generate_address(sc);
    return sc->ci;
}


void generate_class_addresses(sharp_class *sc) {
    generate_address(sc);

    for(Int i = 0; i < sc->fields.size(); i++) {
        sharp_field *sf = sc->fields.get(i);

        if(sf->used) {
            generate_address(sf);
        }
    }

    for(Int i = 0; i < sc->functions.size(); i++) {
        sharp_function *sf = sc->functions.get(i);

        if(sf->used) {
            generate_address(sf);
        }
    }

    for(Int i = 0; i < sc->children.size(); i++) {
        sharp_class *child = sc->children.get(i);

        if(child->used) {
            generate_class_addresses(child);
        }
    }
}