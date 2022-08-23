//
// Created by bknun on 8/23/2022.
//

#include "scheme_continue.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_continue_scheme(operation_schema *scheme) {
    process_scheme_steps(scheme);
}
