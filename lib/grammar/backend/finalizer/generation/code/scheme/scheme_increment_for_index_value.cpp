//
// Created by bknun on 8/22/2022.
//

#include "scheme_increment_for_index_value.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_increment_for_index_value_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_get_local_field_value(next_step);
    process_increment_value(next_step);
}
