//
// Created by bknun on 8/16/2022.
//

#include "scheme_assign_array_value.h"
#include "scheme_processor.h"
#include "../code_context.h"

void process_assign_array_value_scheme(operation_schema *scheme) {
    clear_machine_data();
    process_scheme_steps(scheme);
}
