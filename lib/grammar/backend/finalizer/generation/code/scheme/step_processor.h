//
// Created by bknun on 8/9/2022.
//

#ifndef SHARP_STEP_PROCESSOR_H
#define SHARP_STEP_PROCESSOR_H

#include "../../../../operation/operation.h"
#include "../../../../../../runtime/Opcode.h"

void add_instruction(opcode_instr *instr);
void add_instruction(opcode_instr instr);

void process_get_static_class_instance(operation_step *step);
void process_get_instance_field_value(operation_step *step);
void process_get_primary_class_instance(operation_step *step);
void process_get_tls_field_value(operation_step *step);
void process_scheme(operation_step *step);
void process_record_line_info(operation_step *step);
void process_return_number(operation_step *step);
void process_get_value(operation_step *step);
void process_get_integer_constant(operation_step *step);
void process_get_bool_constant(operation_step *step);
void process_get_decimal_constant(operation_step *step);
void process_not_value(operation_step *step);
void process_negate_value(operation_step *step);
void process_increment_value(operation_step *step);
void process_decrement_value(operation_step *step);
void process_static_function_call(operation_step *step);
void process_instance_function_call(operation_step *step);
void process_push_parameter_to_stack(operation_step *step);
void process_create_class(operation_step *step);
void process_duplicate_item(operation_step *step);
void process_push_value_to_stack(operation_step *step);
void process_allocate_register(operation_step *step);
void process_deallocate_register(operation_step *step);
void process_retain_numeric_value(operation_step *step);
void process_call_dynamic_function(operation_step *step);

void process_step(operation_step *step);

#endif //SHARP_STEP_PROCESSOR_H
