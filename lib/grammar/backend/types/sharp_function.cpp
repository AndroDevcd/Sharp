//
// Created by BNunnally on 9/1/2021.
//

#include "sharp_function.h"
#include "sharp_field.h"

bool is_fully_qualified_function(sharp_function* function) {
    if(!function->parameters.empty()) {
        for(Int i = 0; i < function->parameters.size(); i++) {
            sharp_field *parameter = function->parameters.get(i);
            if(parameter->type.type >= type_any)
                return false;
        }
    }

    return true;
}

bool function_parameters_match(List<sharp_field*> &comparer, List<sharp_field*> &comparee, bool explicitMatch) {
    if(comparer.size() != comparee.size()) return false;

    for(Int i = 0; i < comparer.size(); i++) {
        if(directMatch && )
    }
}