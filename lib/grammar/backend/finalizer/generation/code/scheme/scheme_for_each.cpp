//
// Created by bknun on 8/22/2022.
//

#include "scheme_for_each.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_increment_for_index_value.h"
#include "scheme_assign_value.h"
#include "scheme_for_each_position_check.h"

void process_for_each_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_allocate_label(next_step);

    // set index field to -1
    process_assign_value_scheme(next_step->scheme);

    // assign result field to array expression
    process_assign_value_scheme(next_step->scheme);

    process_set_label(next_step);

    // process index field increment
    process_increment_for_index_value_scheme(next_step->scheme);

    // check if we have processed all items
    process_for_each_position_check_scheme(next_step->scheme);

    process_assign_value_scheme(next_step->scheme);

    // master scheme block
    process_step(next_step);

    process_jump(next_step);
    process_set_label(next_step);
}
