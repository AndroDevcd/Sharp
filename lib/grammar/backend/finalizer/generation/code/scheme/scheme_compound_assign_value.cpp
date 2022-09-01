//
// Created by bknun on 8/20/2022.
//

#include "scheme_compound_assign_value.h"
#include "step_processor.h"
#include "../code_context.h"
#include "../../../../types/types.h"
#include "../code_info.h"
#include "scheme_processor.h"

void process_compound_assign_value_scheme(operation_schema *scheme) {
    sharp_field *closureRef = NULL, *closure = NULL;
    Int stepPos = 0;

    process_allocate_register(next_step);
    process_allocate_register(next_step);
    process_get_value(next_step);
    process_retain_numeric_value(next_step);

    process_get_value(next_step);
    machine_data asignee(cc.machineData);

    if((cc.machineData.type == local_field_object_data || cc.machineData.type == numeric_local_field)
       && cc.container->locals.get(cc.machineData.dataAddress)->closure != NULL) {
        closureRef = cc.container->locals.get(cc.machineData.dataAddress)->closureRef;
        closure = cc.container->locals.get(cc.machineData.dataAddress)->closure;
    }

    process_retain_numeric_value(next_step);

    process_step(next_step); // add, sub, mult, div, mod, etc

    cc.machineData.field = asignee.field;
    cc.machineData.dataAddress = asignee.dataAddress;
    cc.machineData.type = asignee.type;

    process_assign_numeric_value(next_step);

    process_deallocate_register(next_step);
    process_deallocate_register(next_step);

    if(closureRef && closure) {
        push_machine_data_to_stack();
        add_instruction(Opcode::Builder::dup());

        add_instruction(Opcode::Builder::movg(closureRef->owner->ci->address));
        add_instruction(Opcode::Builder::movn(closureRef->ci->address));
        add_instruction(Opcode::Builder::movn(closure->ci->address));

        set_machine_data(closure, false);
        pop_machine_data_from_stack();

        if(is_numeric_type(closure->closure->type) && !closure->type.isArray) {
            set_machine_data(get_register(EBX));
            pop_machine_data_from_stack();
            set_machine_data(get_register(EBX));
        } else {
            add_instruction(Opcode::Builder::popObject2());
            set_machine_data(generic_object_data);
        }
    }
}
