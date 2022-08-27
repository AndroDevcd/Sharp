//
// Created by bknun on 8/26/2022.
//

#include "scheme_and.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_and_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_get_value(next_step);

    process_jump_if_false(next_step);
    process_get_value(next_step);
    consume_machine_data(get_register(CMT));
    set_machine_data(get_register(CMT));
    process_set_label(next_step);
}