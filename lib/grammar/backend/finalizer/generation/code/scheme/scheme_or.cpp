//
// Created by bknun on 8/20/2022.
//

#include "scheme_or.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"

void process_or_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(scheme->steps.get(stepPos++));
    process_get_value(scheme->steps.get(stepPos++));

    process_jump_if_true(scheme->steps.get(stepPos++));
    process_get_value(scheme->steps.get(stepPos++));
    consume_machine_data(get_register(CMT));
    set_machine_data(get_register(CMT));
    process_set_label(scheme->steps.get(stepPos++));
}
