//
// Created by bknun on 8/22/2022.
//

#include "scheme_elseif.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_elseif_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_get_value(next_step);
    process_jump_if_false(next_step);

    // master scheme for if block
    process_step(next_step);

    if(scheme->steps.get(stepPos)->type == operation_jump) {
        process_jump(next_step);
    }

    process_set_label(next_step);
}
