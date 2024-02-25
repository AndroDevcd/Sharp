//
// Created by bknun on 8/19/2022.
//

#include "scheme_null_fallback.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../generator.h"

void process_null_fallback_scheme(operation_schema *scheme) {
    process_allocate_label(scheme->steps.get(0));
    process_get_value(scheme->steps.get(1));

    consume_machine_data();
    add_instruction(Opcode::Builder::checkNull(CMT));

    Int label = create_label();
    create_dynamic_instruction(
            dynamic_instruction(Opcode::JNE, 1,
                       dynamic_argument(label_argument, label)
            )
    );

    process_set_label(scheme->steps.get(2));
    process_get_value(scheme->steps.get(3));
    consume_machine_data();
    set_machine_data(generic_object_data);

    set_label(find_label(label));
    if(scheme->steps.size() > 4){
        for(Int i = 4; i < scheme->steps.size(); i++) {
            process_step(scheme->steps.get(i));
        }
    }
}
