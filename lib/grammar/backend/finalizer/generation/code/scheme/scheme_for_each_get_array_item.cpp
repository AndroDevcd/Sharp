//
// Created by bknun on 8/22/2022.
//

#include "scheme_for_each_get_array_item.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_assign_value.h"

void process_for_each_get_array_item_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_assign_value_scheme(next_step->scheme);
}
