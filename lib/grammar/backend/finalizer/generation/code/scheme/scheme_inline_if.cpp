//
// Created by bknun on 8/19/2022.
//

#include "scheme_inline_if.h"
#include "scheme_processor.h"
#include "step_processor.h"
#include "../code_context.h"

void process_inline_if_scheme(operation_schema *scheme) {
    bool isNumeric = is_numeric_type(scheme->type) && !scheme->type.isArray;
    int stepPos = 0;

    process_allocate_label(next_step);
    process_allocate_label(next_step);
    process_get_value(next_step);
    process_jump_if_false(next_step);
    process_get_value(next_step);

    if(isNumeric) {
        consume_machine_data(get_register(EBX));
    } else
        consume_machine_data();

    process_jump(next_step);
    process_set_label(next_step);
    process_get_value(next_step);

    if(isNumeric) {
        consume_machine_data(get_register(EBX));
    } else
        consume_machine_data();
    process_set_label(next_step);

    if(isNumeric)
        set_machine_data(get_register(EBX));
    else set_machine_data(generic_object_data);
}