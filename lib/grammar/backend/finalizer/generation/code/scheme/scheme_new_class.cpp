//
// Created by bknun on 8/15/2022.
//

#include "scheme_new_class.h"
#include "step_processor.h"
#include "../code_context.h"

void process_new_class_scheme(operation_schema *scheme) {
    process_create_class(scheme->steps.get(0));
    process_duplicate_item(scheme->steps.get(1));
    process_scheme(scheme->steps.get(2));

    set_machine_data(new_class_data);
    if(scheme->steps.size() > 3){
        for(Int i = 3; i < scheme->steps.size(); i++) {
            process_step(scheme->steps.get(i));
        }
    }
}
