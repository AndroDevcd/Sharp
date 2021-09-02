//
// Created by BNunnally on 9/2/2021.
//

#include "sharp_type.h"
#include "sharp_class.h"
#include "sharp_field.h"
#include "sharp_function.h"

// the comparer is the one to receive the value that the comparee holds
bool is_explicit_type_match(sharp_type comparer, sharp_type comparee) {
    if(
         (comparer.type == comparer.type)
         || (comparer.fun != NULL && comparee.fun != NULL)
    ) {
        if(comparer.isArray != comparee.isArray) return false;

        if(comparer.fun == NULL) {
            if(comparer.type == type_class) {
                return is_explicit_type_match(
                        comparer._class, comparee._class
                    );
            } else if(comparer.type == type_field) {
                return is_explicit_type_match(
                        comparer.field->type, comparer.field->type
                    );
            } else return true;
        } else return function_parameters_match(
                comparer.fun->parameters, comparee.fun->parameters, true
                );
    } else return false;
}

bool is_implicit_type_match(sharp_type comparer, sharp_type comparee) {

}
