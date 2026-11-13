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
 * A call through a function value:   f(a, b)
 *
 *   (steps that fetch f)                              f is an object
 *   one step      operation_push_value_to_stack       push f
 *   n steps       operation_push_parameter_to_stack   push a ; push b
 *   last step     operation_call_dynamic_function     calld 2
 *
 * The function value waits on the stack under the arguments, where nothing a
 * parameter does can reach it - f(f(x)) is fine. (When a function value was a
 * number it travelled in a register, and had to be parked in a frame slot
 * while the parameters were evaluated.)
 */
void process_call_dynamic_function_scheme(operation_schema *scheme){
    process_scheme_steps(scheme);
}
