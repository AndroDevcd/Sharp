//
// Created by bknun on 8/20/2022.
//

#include "scheme_instance_check.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_instance_check_scheme(operation_schema *scheme) {
    Int stepPos = 0;
    process_get_value(next_step);
    process_push_value_to_stack(next_step);
    process_get_value(next_step);
    process_push_value_to_stack(next_step);
    process_step(next_step);
}
