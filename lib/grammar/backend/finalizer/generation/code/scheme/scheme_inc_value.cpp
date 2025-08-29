//
// Created by bknun on 8/14/2022.
//

#include "scheme_inc_value.h"
#include "step_processor.h"

void process_increment_value_scheme(operation_schema *scheme) {
    if(scheme->steps.get(0)->type == operation_get_integer_constant
       || scheme->steps.get(0)->type == operation_get_bool_constant
       || scheme->steps.get(0)->type == operation_get_decimal_constant) {
        process_step(scheme->steps.get(0));
    } else {
        process_get_value(scheme->steps.get(0));
        process_increment_value(scheme->steps.get(1));
    }
}
