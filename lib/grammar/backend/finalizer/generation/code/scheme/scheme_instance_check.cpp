//
// Created by bknun on 8/20/2022.
//

#include "scheme_instance_check.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"

void process_instance_check_scheme(operation_schema *scheme) {
    Int stepPos = 0;
    process_get_value(scheme->steps.get(stepPos++));
    process_push_value_to_stack(scheme->steps.get(stepPos++));
    process_get_value(scheme->steps.get(stepPos++));
    process_push_value_to_stack(scheme->steps.get(stepPos++));
    process_step(scheme->steps.get(stepPos++));
}
