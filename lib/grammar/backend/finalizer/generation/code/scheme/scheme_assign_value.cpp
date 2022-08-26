//
// Created by bknun on 8/20/2022.
//

#include "scheme_assign_value.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_assign_value_scheme(operation_schema *scheme) {
    sharp_field *closureRef = NULL, *closure = NULL;
    Int stepPos = 0;

    process_get_value(next_step);
    process_push_value_to_stack(next_step);

    process_get_value(next_step);
    machine_data asignee(cc.machineData);

    if((cc.machineData.type == local_field_object_data || cc.machineData.type == numeric_local_field)
       && cc.container->locals.get(cc.machineData.dataAddress)->closure != NULL) {
        closureRef = cc.container->locals.get(cc.machineData.dataAddress)->closureRef;
        closure = cc.container->locals.get(cc.machineData.dataAddress)->closure;
    }

    process_pop_value_from_stack(next_step);

    cc.machineData.dataAddress = asignee.dataAddress;
    cc.machineData.type = asignee.type;

    bool clearData = false;
    if(has_next_step && current_step->type == operation_unused_data) {
        clearData = true;
        process_unused_data(next_step);
    } else if(has_next_step && current_step->type >= operation_int8_cast
        && current_step->type <= operation_uint64_cast)
        process_var_cast(next_step);
    else  if(has_next_step && current_step->type == operation_cast_class)
        process_class_cast(next_step);

    if(closureRef && closure) {
        cc.machineData.dataAddress = asignee.dataAddress;
        cc.machineData.type = asignee.type;

        push_machine_data_to_stack();
        if(!clearData) {
            add_instruction(Opcode::Builder::dup());
        }

        add_instruction(Opcode::Builder::movg(closureRef->owner->ci->address));
        add_instruction(Opcode::Builder::movn(closureRef->ci->address));
        add_instruction(Opcode::Builder::movn(closure->ci->address));

        set_machine_data(closure, false);
        pop_machine_data_from_stack();

        if(!clearData) {
            if (is_numeric_type(closure->closure->type) && !closure->type.isArray) {
                set_machine_data(get_register(EBX));
                pop_machine_data_from_stack();
                set_machine_data(get_register(EBX));
            } else {
                add_instruction(Opcode::Builder::popObject2());
                set_machine_data(generic_object_data);
            }
        }
    }
}
