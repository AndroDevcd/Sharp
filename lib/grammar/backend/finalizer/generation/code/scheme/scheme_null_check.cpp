//
// Created by bknun on 8/20/2022.
//

#include "scheme_null_check.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_null_check_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_get_value(next_step);

    consume_machine_data();
    process_check_null(next_step);

    if(scheme->steps.size() >= (stepPos + 1)) {
        process_not_value(next_step);
    }
}
