//
// Created by bknun on 8/23/2022.
//

#include "scheme_last_finally_clause.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_master.h"
#include "scheme_unlock_data.h"
#include "scheme_return_address_check_1.h"

void process_last_finally_scheme(operation_schema *scheme) {
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

    process_check_null(next_step);
    process_jump_if_true(next_step);

    for(Int i = stepPos; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);

        if(step->type == operation_step_scheme
            && step->scheme->schemeType == scheme_unlock_data) {
            process_unlock_data_scheme(step->scheme);
        } else {
            stepPos = i;
            break;
        }
    }

    process_get_local_field_value(next_step);
    process_push_value_to_stack(next_step);
    process_return_with_error_state(next_step);

    process_set_label(next_step);

    process_return_address_check_1_scheme(next_step->scheme);
    process_set_label(next_step);

    process_set_finally_end(next_step);
}
