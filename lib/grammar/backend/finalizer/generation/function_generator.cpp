//
// Created by bknun on 8/11/2022.
//

#include "../../dependency/closure_capture.h"
#include "class_generator.h"
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
//        if(sf->type == delegate_function)
//            sf->ci->address = delegateFunctionAddressCounter++;
//        else
            sf->ci->address = functionAddressCounter++;
        compressedCompilationFunctions.addif(sf);

        // A lambda that captures takes the closure object as a hidden LAST
        // parameter (dependency/closure_capture.h). It was kept out of the
        // parameter list while types were matched; from here on the list is
        // only used to lay out the frame and to describe the function in the
        // executable, and there it is a parameter like the others.
        if(sf->capturedParameter != NULL)
            sf->parameters.addif(sf->capturedParameter);
    }
}

void generate(sharp_function *sf) {
    if(sf->type == delegate_function || sf->type == native_function) {
        if(sf->type == native_function) {
            update_context(sf->ci, sf);
            flush_context(); // generate stack offsets
        }
        return;
    }

    for(Int i = 0; i < sf->locals.size(); i++) {
        // A function that is generated needs a frame slot for every one of its
        // locals. generate_address() only gives one to locals marked as used,
        // and the marking runs once per function: a local added afterwards -
        // the second `init { }` block of a class is compiled into an init
        // function that may already be marked - had no slot, and the generator
        // crashed on it (segmentation fault, no message).
        sf->locals.get(i)->used = true;
        generate_address(sf->locals.get(i), i, check_flag(sf->flags, flag_static));
    }

    process_scheme(sf->scheme, sf->ci, sf);
}

/**
 * closure.<field> = the object in frame slot `slot`
 *
 *      pushl  slot
 *      movl   __closure
 *      movn   field
 *      popobj
 */
static void store_slot_in_closure(sharp_field *closureLocal, sharp_field *field, Int slot) {
    add_instruction(Opcode::Builder::pushl(slot));
    add_instruction(Opcode::Builder::movl(closureLocal->ci->address));
    add_instruction(Opcode::Builder::movn(get_or_initialize_code(field)->address));
    add_instruction(Opcode::Builder::popObject());
}

/**
 * The first instructions of a function that lambdas capture from
 * (dependency/closure_capture.h): make this call's closure object and fill in
 * what is known at the start.
 *
 *      __closure = new closure_<function>
 *      __closure.__parent = __captured     a lambda around other lambdas
 *      __closure.__@self  = self           an instance function
 *      __closure.<p>      = <p>            every captured parameter
 *
 * (Captured locals are written when the function assigns them:
 *  scheme_assign_value.cpp.)
 */
void generate_initial_closure_setup(sharp_function *sf) {
    sharp_field *closureLocal = sf->closure;
    if(closureLocal == NULL || closureLocal->ci == NULL)
        return;

    sharp_class *closureClass = closureLocal->type._class;
    if(closureClass == NULL)
        return;

    add_instruction(Opcode::Builder::newClass(get_or_initialize_code(closureClass)->address));
    add_instruction(Opcode::Builder::popl(closureLocal->ci->address));

    sharp_field *parent = locate_field(closure_parent_field_name, closureClass);
    if(parent != NULL && sf->capturedParameter != NULL)
        store_slot_in_closure(closureLocal, parent, sf->capturedParameter->ci->address);

    sharp_field *self = locate_field(closure_self_field_name, closureClass);
    if(self != NULL && !check_flag(sf->flags, flag_static))
        store_slot_in_closure(closureLocal, self, 0);    // slot 0: the instance

    for(Int i = 0; i < sf->parameters.size(); i++) {
        sharp_field *field = sf->parameters.get(i);

        if(field->closure != NULL && field->closureRef != NULL) {
            sharp_field *closure = field->closure;
            set_machine_data(field, true);

            push_machine_data_to_stack();

            add_instruction(Opcode::Builder::movl(closureLocal->ci->address));
            move_to_field(closure);

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
