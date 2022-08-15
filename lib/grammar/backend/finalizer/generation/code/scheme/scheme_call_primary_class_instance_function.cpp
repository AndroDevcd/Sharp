//
// Created by bknun on 8/15/2022.
//

#include "scheme_call_primary_class_instance_function.h"
#include "step_processor.h"

void process_primary_class_instance_function_scheme(operation_schema *scheme) {
    process_get_primary_class_instance(scheme->steps.get(0));
    process_push_value_to_stack(scheme->steps.get(1));

    for(Int i = 2; i < scheme->steps.size(); i++) {
        if(scheme->steps.get(i)->type == operation_push_parameter_to_stack) {
            process_step(scheme->steps.get(i));
        } else break;
    }

    process_instance_function_call(scheme->steps.last());
}
