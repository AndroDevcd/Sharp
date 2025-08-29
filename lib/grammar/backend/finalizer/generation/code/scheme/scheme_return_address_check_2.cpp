//
// Created by bknun on 8/23/2022.
//

#include "scheme_return_address_check_2.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_return_address_check_2_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_register(next_step);
    process_allocate_register(next_step);

    process_get_value(next_step);
    process_retain_numeric_value(next_step);

    process_get_value(next_step);
    process_retain_numeric_value(next_step);

    process_eq_eq(next_step);
    process_jump_if_false(next_step);

    process_deallocate_register(next_step);
    process_deallocate_register(next_step);
}
