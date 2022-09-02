//
// Created by bknun on 8/23/2022.
//

#include "scheme_finally_clause.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_master.h"
#include "scheme_return_address_check_2.h"

void process_finally_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_set_try_catch_end(next_step);
    process_allocate_finally_data(next_step);

    process_set_label(next_step);
    process_setup_local_field(next_step);
    process_set_label(next_step);
    process_set_finally_start(next_step);
    process_set_finally_exception_field(next_step);

    // process block data
    process_master_scheme(next_step->scheme);

    process_get_local_field_value(next_step);
    consume_machine_data();

    process_check_null(next_step);
    process_jump_if_true(next_step);
    process_jump(next_step);


    process_set_label(next_step);

    process_return_address_check_2_scheme(next_step->scheme);
    process_set_label(next_step);

    process_set_finally_end(next_step);
}