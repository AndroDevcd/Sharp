//
// Created by bknun on 8/23/2022.
//

#include "scheme_catch_clause.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_master.h"

void process_catch_clause_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_catch_data(next_step);

    process_set_catch_class(next_step);
    process_set_catch_field(next_step);
    process_set_catch_start(next_step);

    process_master_scheme(next_step->scheme);

    process_jump(next_step);
}
