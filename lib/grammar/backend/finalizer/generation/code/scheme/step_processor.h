//
// Created by bknun on 8/9/2022.
//

#ifndef SHARP_STEP_PROCESSOR_H
#define SHARP_STEP_PROCESSOR_H

#include "../../../../operation/operation.h"
#include "../../../../../../runtime/Opcode.h"

struct internal_register;

void add_instruction(opcode_instr *instr);
void add_instruction(opcode_instr instr);

void process_get_static_class_instance(operation_step *step);
void process_get_instance_field_value(operation_step *step);
void process_get_local_field_value(operation_step *step);
void process_get_primary_class_instance(operation_step *step);
void process_get_tls_field_value(operation_step *step);
void process_scheme(operation_step *step);
void process_record_line_info(operation_step *step);
void process_return_number(operation_step *step);
void process_return_object(operation_step *step);
void process_return_nil(operation_step *step);
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
void process_pop_value_from_stack(operation_step *step);
void process_allocate_register(operation_step *step);
void process_deallocate_register(operation_step *step);
void process_retain_numeric_value(operation_step *step);
void process_call_dynamic_function(operation_step *step);
void process_get_char_constant(operation_step *step);
void process_get_string_constant(operation_step *step);
void process_get_static_function_address(operation_step *step);
void process_nullify_value(operation_step *step);
void process_get_size_value(operation_step *step);
void process_create_class_array(operation_step *step);
void process_create_number_array(operation_step *step);
void process_create_object_array(operation_step *step);
void process_assign_array_element_from_stack(operation_step *step);
void process_assign_array_value(operation_step *step);
void process_get_size_of(operation_step *step);
void process_cast_class(operation_step *step);
void process_var_class(operation_step *step);
void process_is_class(operation_step *step);
void process_is_type(operation_step *step);
void process_jump_if_false(operation_step *step);
void process_jump_if_true(operation_step *step);
void process_jump(operation_step *step);
void process_set_label(operation_step *step);
void process_allocate_label(operation_step *step);
void process_add(operation_step *step);
void process_sub(operation_step *step);
void process_mult(operation_step *step);
void process_div(operation_step *step);
void process_mod(operation_step *step);
void process_and(operation_step *step);
void process_or(operation_step *step);
void process_xor(operation_step *step);
void process_assign_numeric_value(operation_step *step);
internal_register* get_register_or_consume(Int registerId, bool leftSide);
void process_and_and(operation_step *step);
void process_eq_eq(operation_step *step);
void process_not_eq(operation_step *step);
void process_lt(operation_step *step);
void process_gt(operation_step *step);
void process_gte(operation_step *step);
void process_lte(operation_step *step);
void process_shl(operation_step *step);
void process_shr(operation_step *step);
void process_exponent(operation_step *step);
void process_check_null(operation_step *step);
void process_instance_not_eq(operation_step *step);
void process_instance_eq(operation_step *step);

void process_step(operation_step *step);

#endif //SHARP_STEP_PROCESSOR_H
