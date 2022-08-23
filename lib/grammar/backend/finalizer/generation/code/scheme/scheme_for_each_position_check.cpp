//
// Created by bknun on 8/22/2022.
//

#include "scheme_for_each_position_check.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_get_size_of.h"

void process_for_each_position_check_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_register(next_step);
    process_allocate_register(next_step);

    process_retain_numeric_value(next_step);

    process_get_size_of_scheme(next_step->scheme);

    process_retain_numeric_value(next_step);
    process_lt(next_step);

    process_jump_if_false(next_step);
    process_unused_data(next_step);

    process_deallocate_register(next_step);
    process_deallocate_register(next_step);

}
