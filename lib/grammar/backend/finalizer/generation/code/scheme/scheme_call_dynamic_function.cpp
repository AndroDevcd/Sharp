//
// Created by bknun on 8/15/2022.
//

#include "scheme_call_dynamic_function.h"
#include "step_processor.h"
#include "scheme_processor.h"
#include "../code_context.h"
#include "../register_allocator.h"
#include "../../generator.h"
#include "../../../../types/types.h"

/*
 * A call through a function pointer:   f(a, b)
 *
 *   (steps that fetch f)
 *   one step      operation_retain_numeric_value      regs[R] = address held in f
 *   n steps       operation_push_parameter_to_stack   push a ; push b
 *   last step     operation_call_dynamic_function     calld R
 *
 * A parameter may itself contain a call - f(f(x)) - and a callee is free to
 * overwrite every register, R included. So while the parameters are evaluated
 * the address is parked in a frame slot of its own and loaded back right before
 * the call:
 *
 *       loadl   R, fp+f
 *       smovr_2 R, fp+tmp        ; park
 *       ...parameters...
 *       loadl   R, fp+tmp        ; restore
 *       calld   R
 */
void process_call_dynamic_function_scheme(operation_schema *scheme){
    Int count = scheme->steps.size();

    // (the scheme may start with the steps that fetch the pointer; find our three parts)
    Int callIndex = -1, firstParameter = -1, retainIndex = -1;
    for(Int i = 0; i < count; i++) {
        if(scheme->steps.get(i)->type == operation_push_parameter_to_stack && firstParameter < 0)
            firstParameter = i;
        if(scheme->steps.get(i)->type == operation_call_dynamic_function)
            callIndex = i;
    }
    if(firstParameter > 0 && callIndex > firstParameter) {
        for(Int i = firstParameter - 1; i >= 0 && retainIndex < 0; i--) {
            if(scheme->steps.get(i)->type == operation_retain_numeric_value
               && scheme->steps.get(i)->integer == scheme->steps.get(callIndex)->integer)
                retainIndex = i;
        }
    }

    if(retainIndex < 0) {
        process_scheme_steps(scheme);   // no parameters: nothing can overwrite the register
        return;
    }

    for(Int i = 0; i <= retainIndex; i++)
        process_step(scheme->steps.get(i));

    internal_register *address = find_register(scheme->steps.get(retainIndex)->integer);
    internal_register *parked = NULL;

    if(address != NULL && address->type == normal_register) {
        // a frame slot behind the function's locals, exactly like the ones
        // allocate_register() hands out when it runs out of registers
        parked = new internal_register(
                -1, (check_flag(cc.container->flags, flag_static) ? 0 : 1) +
                cc.container->locals.size() + (cc.registers.size() - MIN_REGISTERS),
                stack_register, status_busy);
        cc.registers.add(parked);

        add_instruction(Opcode::Builder::smovr2((_register)address->address, parked->address));
    }

    for(Int i = retainIndex + 1; i < callIndex; i++)
        process_step(scheme->steps.get(i));

    if(parked != NULL) {
        add_instruction(Opcode::Builder::loadl((_register)address->address, parked->address));
        parked->status = status_free;   // the slot can be reused from here on
    }

    for(Int i = callIndex; i < count; i++)
        process_step(scheme->steps.get(i));
}
