//
// Created by bknun on 8/22/2022.
//

#include "scheme_for_cond.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_for_cond_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_get_value(next_step);
    consume_machine_data(get_register(CMT));
    set_machine_data(get_register(CMT));

    process_jump_if_false(next_step);
}
