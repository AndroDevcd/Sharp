//
// Created by bknun on 8/20/2022.
//

#include "scheme_accelerated_binary_math.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_accelerated_binary_math_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_register(next_step);

    process_get_value(next_step);
    process_retain_numeric_value(next_step);

    process_get_value(next_step);
    clear_machine_data();

    process_step(next_step); // add, sub, mult, div, mod, etc

    process_deallocate_register(next_step);
}