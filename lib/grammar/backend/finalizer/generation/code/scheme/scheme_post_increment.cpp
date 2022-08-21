//
// Created by bknun on 8/21/2022.
//

#include "scheme_post_increment.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_post_increment_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    for(Int i = 0; i < scheme->steps.size(); i++) {
        operation_step *step = scheme->steps.get(i);

        if(step->type == operation_post_scheme_start) {
            stepPos++;
            break;
        }
        else process_step(step);
        stepPos++;
    }

    machine_data md(cc.machineData);
    process_push_value_to_stack(scheme->steps.get(stepPos++));

    // this will re-hydrate the machine data type after pushing it to the sack
    cc.machineData.type = md.type;
    cc.machineData.dataAddress = md.dataAddress;
    cc.machineData.field = md.field;
    process_step(scheme->steps.get(stepPos++));

    set_machine_data(get_register(EBX), true);
    process_pop_value_from_stack(scheme->steps.get(stepPos++));
    set_machine_data(get_register(EBX));
}
