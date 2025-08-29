//
// Created by bknun on 6/12/2023.
//

#include "scheme_assign_array_index.h"
#include "scheme_processor.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/sharp_function.h"

void process_assign_array_index_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_get_value(next_step);
    process_push_value_to_stack(next_step);
    process_step(next_step);
}