//
// Created by bknun on 8/20/2022.
//

#include "scheme_binary_math.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"

void process_binary_math_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_register(scheme->steps.get(stepPos++));
    process_allocate_register(scheme->steps.get(stepPos++));

    process_get_value(scheme->steps.get(stepPos++));
    process_retain_numeric_value(scheme->steps.get(stepPos++));


    process_step(scheme->steps.get(stepPos++));
    process_retain_numeric_value(scheme->steps.get(stepPos++));

    process_step(scheme->steps.get(stepPos++)); // add, sub, mult, div, mod, etc

    process_deallocate_register(scheme->steps.get(stepPos++));
    process_deallocate_register(scheme->steps.get(stepPos++));
}
