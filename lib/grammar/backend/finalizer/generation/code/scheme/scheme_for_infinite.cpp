//
// Created by bknun on 8/22/2022.
//

#include "scheme_for_infinite.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_for_infinite(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);

    process_set_label(next_step);

    // master scheme for block
    process_step(next_step);

    process_jump(next_step);
}
