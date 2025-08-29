//
// Created by bknun on 8/22/2022.
//

#include "scheme_when.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"
#include "scheme_when_clause.h"
#include "scheme_when_else_clause.h"
#include "scheme_assign_value.h"

void process_when_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    if(current_step->type != operation_allocate_label)
        process_step(next_step);
    process_allocate_label(next_step);

    for(Int i = stepPos; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);

        if(step->type == operation_step_scheme
           && step->scheme->schemeType == scheme_when_clause) {
            process_when_clause_scheme(step->scheme);
        } else if(step->type == operation_step_scheme
                  && step->scheme->schemeType == scheme_when_else_clause) {
            process_when_else_clause_scheme(step->scheme);
        }
        else {
            stepPos = i;
            break;
        }
    }

    process_set_label(next_step);
}
