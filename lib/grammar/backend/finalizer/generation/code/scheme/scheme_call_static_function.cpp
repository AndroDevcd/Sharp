//
// Created by bknun on 8/14/2022.
//

#include "scheme_call_static_function.h"
#include "step_processor.h"

void process_call_static_function_scheme(operation_schema *scheme) {
    for(Int i = 0; i < scheme->steps.size(); i++) {
        if(scheme->steps.get(i)->type == operation_push_parameter_to_stack) {
            process_step(scheme->steps.get(i));
        } else break;
    }

    process_static_function_call(scheme->steps.last());
}
