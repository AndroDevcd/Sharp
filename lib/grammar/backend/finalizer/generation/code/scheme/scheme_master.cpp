//
// Created by bknun on 8/12/2022.
//

#include "scheme_master.h"
#include "scheme_processor.h"
#include "../../generator.h"
#include "../code_context.h"

void process_master_scheme(operation_schema *scheme) {
    for(Int i = 0; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);

        if(step->type == operation_step_scheme)
            process_scheme(step->scheme);
        else
            generation_error("found non-scheme step in function: " + cc.container->fullName);
    }
}
