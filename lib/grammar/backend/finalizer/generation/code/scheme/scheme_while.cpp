//
// Created by bknun on 8/23/2022.
//

#include "scheme_while.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_master.h"

void process_while_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_allocate_label(next_step);

    process_set_label(next_step);
    process_get_value(next_step);

    process_jump_if_false(next_step);

    // process when block
    process_master_scheme(next_step->scheme);

    process_jump(next_step);
    process_set_label(next_step);
}
