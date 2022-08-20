//
// Created by bknun on 8/11/2022.
//

#include "function_generator.h"
#include "../../types/types.h"
#include "generator.h"
#include "code/scheme/scheme_processor.h"
#include "field_generator.h"

uInt functionAddressCounter = 0;
void generate_address(sharp_function *sf) {
    if(sf->ci == NULL) {
        sf->ci = new code_info();
        sf->ci->uuid = UUIDGenerator++;
        sf->ci->address = functionAddressCounter++;
        compressedCompilationFunctions.addif(sf);
    }
}

code_info* get_or_initialize_code(sharp_function *sf) {
    generate_address(sf);
    return sf->ci;
}

void generate(sharp_function *sf) {
    for(Int i = 0; i < sf->locals.size(); i++) {
        generate_address(sf->locals.get(i), i);
    }

    if(sf->name == "foo")
        process_scheme(sf->scheme, sf->ci, sf);


    if(sf->name == "foo") {
        cout << "code:\n\n" << code_to_string(sf->ci);
    }
}
