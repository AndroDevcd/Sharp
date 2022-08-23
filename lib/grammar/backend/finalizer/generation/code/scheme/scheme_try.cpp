//
// Created by bknun on 8/23/2022.
//

#include "scheme_try.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_master.h"
#include "scheme_catch_clause.h"
#include "scheme_finally_clause.h"
#include "scheme_last_finally_clause.h"

void process_try_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_allocate_label(next_step);
    process_allocate_label(next_step);
    process_allocate_label(next_step);
    process_allocate_label(next_step);

    process_allocate_try_catch_data(next_step);
    process_set_try_catch_start(next_step);
    process_set_try_catch_block_start(next_step);
    process_setup_local_field(next_step);

    process_master_scheme(next_step->scheme);

    process_set_try_catch_block_end(next_step);

    if(has_next_step && current_step->type == operation_jump)
        process_jump(next_step);

    bool finallyFound = false;
    for(Int i = stepPos; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);

        if(step->type == operation_step_scheme
           && step->scheme->schemeType == scheme_catch_clause) {
            process_catch_clause_scheme(step->scheme);
        } else if(step->type == operation_step_scheme
            && step->scheme->schemeType == scheme_finally_clause) {
            finallyFound = true;
            process_finally_scheme(step->scheme);
        }  else if(step->type == operation_step_scheme
            && step->scheme->schemeType == scheme_last_finally_clause) {
            finallyFound = true;
            process_last_finally_scheme(step->scheme);
        } else {
            stepPos = i;
            break;
        }
    }

    if(!finallyFound) {
        process_set_label(next_step);
        process_set_try_catch_end(next_step);
    }
}
