//
// Created by bknun on 8/8/2022.
//

#include "class_generator.h"
#include "../../types/types.h"

uInt classAddressCounter = 0;
void generate_address(sharp_class *sc) {
    if(sc->ci == NULL) {
        sc->ci = new code_info();
    }

    sc->ci->address = classAddressCounter++;
}

code_info* get_or_initialize_code(sharp_class *sc) {
    if(sc->ci == NULL) {
        generate_address(sc);
    }

    return sc->ci;
}
