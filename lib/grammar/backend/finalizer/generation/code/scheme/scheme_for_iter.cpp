//
// Created by bknun on 8/22/2022.
//

#include "scheme_for_iter.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"

void process_for_iter_scheme(operation_schema *scheme) {
    process_get_value(scheme->steps.get(0));
    clear_machine_data();
}
