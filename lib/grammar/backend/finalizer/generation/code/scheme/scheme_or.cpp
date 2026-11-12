//
// Created by bknun on 8/20/2022.
//

#include "scheme_or.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_or_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    // Steps come from create_or_or_operation() (operation.cpp), in this order.
    process_allocate_label(next_step);      // endLabel
    process_allocate_label(next_step);      // trueLabel
    process_get_value(next_step);           // left

    process_jump_if_true(next_step);        // left is true  -> trueLabel
    process_get_value(next_step);           // right
    process_jump_if_false(next_step);       // right is false -> endLabel (CMT is 0)
    process_set_label(next_step);           // trueLabel:
    Int address = create_constant(1);       //   the result of || is 1, never
    set_machine_data(numeric_constant, address); // the operand that was true
    consume_machine_data(get_register(CMT));
    process_set_label(next_step);           // endLabel:

    set_machine_data(get_register(CMT));
}
