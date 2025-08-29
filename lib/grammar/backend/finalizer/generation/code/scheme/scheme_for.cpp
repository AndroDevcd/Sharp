//
// Created by bknun on 8/22/2022.
//

#include "scheme_for.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_for_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_allocate_label(next_step);
    process_allocate_label(next_step);

    if(scheme->steps.get(stepPos)->type == operation_step_scheme
        && scheme->steps.get(stepPos)->scheme->schemeType == scheme_master) {
        // variable decl
        process_step(next_step);
        clear_machine_data();
    }

    process_set_label(next_step);

    if(scheme->steps.get(stepPos)->type == operation_step_scheme
       && scheme->steps.get(stepPos)->scheme->schemeType == scheme_for_cond) {
        // for condition
        process_step(next_step);
        clear_machine_data();
    }

    // master scheme for block
    process_step(next_step);

    process_set_label(next_step);
    if(scheme->steps.get(stepPos)->type == operation_step_scheme
       && scheme->steps.get(stepPos)->scheme->schemeType == scheme_for_iter) {
        // for iter
        process_step(next_step);
    }

    process_jump(next_step);
    process_set_label(next_step);
}
