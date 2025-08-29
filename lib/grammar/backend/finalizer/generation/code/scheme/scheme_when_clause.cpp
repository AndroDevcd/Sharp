//
// Created by bknun on 8/22/2022.
//

#include "scheme_when_clause.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_when_clause_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_allocate_label(next_step);

    for(Int i = stepPos; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);

        if(step->type == operation_get_value) {
            process_get_value(step);
        } else if(step->type == operation_jump_if_true) {
            process_jump_if_true(step);
        }
        else {
            stepPos = i;
            break;
        }
    }

    process_jump_if_false(next_step);
    process_set_label(next_step);

    // when clause block master scheme
    process_step(next_step);

    if(current_step->type == operation_jump) {
        process_jump(next_step);
    }

    process_set_label(next_step);
}
