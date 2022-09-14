//
// Created by bknun on 8/11/2022.
//

#include "function_generator.h"
#include "../../types/types.h"
#include "generator.h"
#include "code/scheme/scheme_processor.h"
#include "field_generator.h"
#include "code/code_context.h"
#include "code/scheme/step_processor.h"

uInt functionAddressCounter = 0;
uInt delegateFunctionAddressCounter = 0;
void generate_address(sharp_function *sf) {
    if(sf->ci == NULL) {
        sf->ci = new code_info();
        sf->ci->uuid = UUIDGenerator++;
        if(sf->type == delegate_function)
            sf->ci->address = delegateFunctionAddressCounter++;
        else
            sf->ci->address = functionAddressCounter++;
        compressedCompilationFunctions.addif(sf);
    }
}

void generate(sharp_function *sf) {
    if(sf->type == delegate_function) {
        return;
    }

    for(Int i = 0; i < sf->locals.size(); i++) {
        generate_address(sf->locals.get(i), i, check_flag(sf->flags, flag_static));
    }

    process_scheme(sf->scheme, sf->ci, sf);
    cout << sf->fullName << " code:\n\n" << code_to_string(sf->ci) << endl;
}

void generate_initial_closure_setup(sharp_function *sf) {
    for(Int i = 0; i < sf->parameters.size(); i++) {
        sharp_field *field = sf->parameters.get(i);

        if(field->closure != NULL && field->closureRef != NULL) {
            sharp_field *closure = field->closure;
            sharp_field *closureRef = field->closureRef;
            set_machine_data(field, true);

            push_machine_data_to_stack();

            add_instruction(Opcode::Builder::movg(closureRef->owner->ci->address));
            add_instruction(Opcode::Builder::movn(closureRef->ci->address));
            add_instruction(Opcode::Builder::movn(closure->ci->address));

            set_machine_data(closure, false);
            pop_machine_data_from_stack();
        }
    }
}

void generate_static_class_setup() {
    for(Int i = 0; i < compressedCompilationClasses.size(); i++) {
        sharp_class *sc = compressedCompilationClasses.get(i);

        if(sc->staticInit != NULL && sc->staticInit->used) {
            add_instruction(Opcode::Builder::call(sc->staticInit->ci->address));
        }
    }
}
