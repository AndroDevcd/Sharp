//
// Created by BNunnally on 9/9/2021.
//

#include "operation.h"
#include "../types/types.h"
#include "../../compiler_info.h"


Int scheme_recursion = -1;
Int space_count = 0;
operation_step *currentStep = NULL;
void add_spaces(Int count, stringstream &ss) {
    for(Int i = 0; i < count; i++) {
        ss << ' ';
    }
}

void add_recursion_tabs(stringstream &ss) {
    add_spaces(space_count, ss);
    for(Int i = 0; i < scheme_recursion; i++) {
        ss << '\t';
    }
}

string operation_scheme_to_str(operation_schema *schema);

void operation_step_to_str(Int index, operation_step *step, stringstream &ss) {
    Int oldSpaceCount = space_count;
    space_count = 4;
    add_recursion_tabs(ss);
    if(step == currentStep) {
        ss << "-> ";
    }
    ss << "operation_step #" << index << ": ";

    switch(step->type) {
       case operation_none:
           ss << "operation_none";
           break;
       case operation_step_scheme:
           ss << "operation_step_scheme";
           break;
       case operation_get_local_field_value:
           ss << "operation_get_local_field_value";
           break;
       case operation_get_instance_field_value:
           ss << "operation_get_instance_field_value";
           break;
       case operation_get_static_class_instance:
           ss << "operation_get_static_class_instance";
           break;
       case operation_get_primary_class_instance:
           ss << "operation_get_primary_class_instance";
           break;
       case operation_get_tls_field_value:
           ss << "operation_get_tls_field_value";
           break;
       case operation_call_instance_function:
           ss << "operation_call_instance_function";
           break;
       case operation_call_static_function:
           ss << "operation_call_static_function";
           break;
       case operation_call_dynamic_function:
           ss << "operation_call_dynamic_function";
           break;
       case operation_get_static_function_address:
           ss << "operation_get_static_function_address";
           break;
       case operation_push_value_to_stack:
           ss << "operation_push_value_to_stack";
           break;
       case operation_post_scheme_start:
           ss << "operation_post_scheme_start";
           break;
       case operation_assign_array_value:
           ss << "operation_assign_array_value";
           break;
       case operation_get_value:
           ss << "operation_get_value";
           break;
       case operation_post_access:
           ss << "operation_post_access";
           break;
       case operation_get_sizeof:
           ss << "operation_get_sizeof";
           break;
       case operation_throw_exception:
           ss << "operation_throw_exception";
           break;
       case operation_pop_value_from_stack:
           ss << "operation_pop_value_from_stack";
           break;
       case operation_pop_direct_numeric_value_from_stack:
           ss << "operation_pop_direct_numeric_value_from_stack";
           break;
       case operation_pop_direct_object_value_from_stack:
           ss << "operation_pop_direct_object_value_from_stack";
           break;
       case operation_set_current_data_as_object:
           ss << "operation_set_current_data_as_object";
           break;
       case operation_unused_data:
           ss << "operation_unused_data";
           break;
       case operation_assign_array_element_from_stack:
           ss << "operation_assign_array_element_from_stack";
           break;
       case operation_nullify_value:
           ss << "operation_nullify_value";
           break;
       case operation_push_parameter_to_stack:
           ss << "operation_push_parameter_to_stack";
           break;
       case operation_get_integer_constant:
           ss << "operation_get_integer_constant";
           break;
       case operation_get_decimal_constant:
           ss << "operation_get_decimal_constant";
           break;
       case operation_negate_value:
           ss << "operation_negate_value";
           break;
       case operation_create_class:
           ss << "operation_create_class";
           break;
       case operation_duplicate_item:
           ss << "operation_duplicate_item";
           break;
       case operation_int8_increment:
           ss << "operation_int8_increment";
           break;
       case operation_int16_increment:
           ss << "operation_int16_increment";
           break;
       case operation_int32_increment:
           ss << "operation_int32_increment";
           break;
       case operation_int64_increment:
           ss << "operation_int64_increment";
           break;
       case operation_uint8_increment:
           ss << "operation_uint8_increment";
           break;
       case operation_uint16_increment:
           ss << "operation_uint16_increment";
           break;
       case operation_uint32_increment:
           ss << "operation_uint32_increment";
           break;
       case operation_uint64_increment:
           ss << "operation_uint64_increment";
           break;
       case operation_illegal:
           ss << "operation_illegal";
           break;
       case operation_var_increment:
           ss << "operation_var_increment";
           break;
       case operation_int8_decrement:
           ss << "operation_int8_decrement";
           break;
       case operation_int16_decrement:
           ss << "operation_int16_decrement";
           break;
       case operation_int32_decrement:
           ss << "operation_int32_decrement";
           break;
       case operation_int64_decrement:
           ss << "operation_int64_decrement";
           break;
       case operation_uint8_decrement:
           ss << "operation_uint8_decrement";
           break;
       case operation_uint16_decrement:
           ss << "operation_uint16_decrement";
           break;
       case operation_uint32_decrement:
           ss << "operation_uint32_decrement";
           break;
       case operation_uint64_decrement:
           ss << "operation_uint64_decrement";
           break;
       case operation_var_decrement:
           ss << "operation_var_decrement";
           break;
       case operation_get_char_constant:
           ss << "operation_get_char_constant";
           break;
       case operation_get_bool_constant:
           ss << "operation_get_bool_constant";
           break;
       case operation_get_string_constant:
           ss << "operation_get_string_constant";
           break;
       case operation_not_value:
           ss << "operation_not_value";
           break;
       case operation_create_class_array:
           ss << "operation_create_class_array";
           break;
       case operation_create_number_array:
           ss << "operation_create_number_array";
           break;
       case operation_create_object_array:
           ss << "operation_create_object_array";
           break;
       case operation_get_size_value:
           ss << "operation_get_size_value";
           break;
       case operation_cast_class:
           ss << "operation_cast_class";
           break;
       case operation_int8_cast:
           ss << "operation_int8_cast";
           break;
       case operation_int16_cast:
           ss << "operation_int16_cast";
           break;
       case operation_int32_cast:
           ss << "operation_int32_cast";
           break;
       case operation_int64_cast:
           ss << "operation_int64_cast";
           break;
       case operation_uint8_cast:
           ss << "operation_uint8_cast";
           break;
       case operation_uint16_cast:
           ss << "operation_uint16_cast";
           break;
       case operation_uint32_cast:
           ss << "operation_uint32_cast";
           break;
       case operation_uint64_cast:
           ss << "operation_uint64_cast";
           break;
       case operation_is_class:
           ss << "operation_is_class";
           break;
       case operation_is_int8:
           ss << "operation_is_int8";
           break;
       case operation_is_int16:
           ss << "operation_is_int16";
           break;
       case operation_is_int32:
           ss << "operation_is_int32";
           break;
       case operation_is_int64:
           ss << "operation_is_int64";
           break;
       case operation_is_uint8:
           ss << "operation_is_uint8";
           break;
       case operation_is_uint16:
           ss << "operation_is_uint16";
           break;
       case operation_is_uint32:
           ss << "operation_is_uint32";
           break;
       case operation_is_uint64:
           ss << "operation_is_uint64";
           break;
       case operation_is_var:
           ss << "operation_is_var";
           break;
       case operation_get_array_element_at_index:
           ss << "operation_get_array_element_at_index";
           break;
       case operation_retain_numeric_value:
           ss << "operation_retain_numeric_value";
           break;
       case operation_discard_register:
           ss << "operation_discard_register";
           break;
       case operation_allocate_register:
           ss << "operation_allocate_register";
           break;
       case operation_allocate_label:
           ss << "operation_allocate_label";
           break;
       case operation_allocate_try_catch_data:
           ss << "operation_allocate_try_catch_data";
           break;
       case operation_allocate_catch_data:
           ss << "operation_allocate_catch_data";
           break;
       case operation_allocate_finally_data:
           ss << "operation_allocate_finally_data";
           break;
       case operation_set_catch_class:
           ss << "operation_set_catch_class";
           break;
       case operation_branch:
           ss << "operation_branch";
           break;
       case operation_set_catch_field:
           ss << "operation_set_catch_field";
           break;
       case operation_set_finally_exception_field:
           ss << "operation_set_finally_exception_field";
           break;
       case operation_set_catch_start:
           ss << "operation_set_catch_start";
           break;
       case operation_set_finally_start:
           ss << "operation_set_finally_start";
           break;
       case operation_set_finally_end:
           ss << "operation_set_finally_end";
           break;
       case operation_and:
           ss << "operation_and";
           break;
       case operation_xor:
           ss << "operation_xor";
           break;
       case operation_or:
           ss << "operation_or";
           break;
       case operation_and_and:
           ss << "operation_and_and";
           break;
       case operation_eq_eq:
           ss << "operation_eq_eq";
           break;
       case operation_not_eq:
           ss << "operation_not_eq";
           break;
       case operation_instance_eq:
           ss << "operation_instance_eq";
           break;
       case operation_instance_not_eq:
           ss << "operation_instance_not_eq";
           break;
       case operation_gt:
           ss << "operation_gt";
           break;
       case operation_lt:
           ss << "operation_lt";
           break;
       case operation_gte:
           ss << "operation_gte";
           break;
       case operation_lte:
           ss << "operation_lte";
           break;
       case operation_shr:
           ss << "operation_shr";
           break;
       case operation_shl:
           ss << "operation_shl";
           break;
       case operation_add:
           ss << "operation_add";
           break;
       case operation_sub:
           ss << "operation_sub";
           break;
       case operation_div:
           ss << "operation_div";
           break;
       case operation_mod:
           ss << "operation_mod";
           break;
       case operation_mult:
           ss << "operation_mult";
           break;
       case operation_exponent:
           ss << "operation_exponent";
           break;
       case operation_assign_numeric_value:
           ss << "operation_assign_numeric_value";
           break;
       case operation_record_line:
           ss << "operation_record_line";
           break;
       case operation_return_nil:
           ss << "operation_return_nil";
           break;
       case operation_return_with_error_state:
           ss << "operation_return_with_error_state";
           break;
       case operation_retain_label_value:
           ss << "operation_retain_label_value";
           break;
       case operation_return_number:
           ss << "operation_return_number";
           break;
       case operation_return_object:
           ss << "operation_return_object";
           break;
       case operation_lock:
           ss << "operation_lock";
           break;
       case operation_unlock:
           ss << "operation_unlock";
           break;
       case operation_set_label:
           ss << "operation_set_label";
           break;
       case operation_jump_if_false:
           ss << "operation_jump_if_false";
           break;
       case operation_jump_if_true:
           ss << "operation_jump_if_true";
           break;
       case operation_jump:
           ss << "operation_jump";
           break;
       case operation_setup_local_field:
           ss << "operation_setup_local_field";
           break;
       case operation_set_try_catch_start:
           ss << "operation_set_try_catch_start";
           break;
       case operation_set_try_catch_end:
           ss << "operation_set_try_catch_end";
           break;
       case operation_set_try_catch_block_start:
           ss << "operation_set_try_catch_block_start";
           break;
       case operation_set_try_catch_block_end:
           ss << "operation_set_try_catch_block_end";
           break;
       case operation_check_null:
           ss << "operation_check_null";
           break;
       case operation_no_op:
           ss << "operation_no_op";
    }

    space_count = 6;
    ss << endl;
    add_recursion_tabs(ss);
    ss << "field: " << (step->field ? step->field->fullName : "null") << endl;

    add_recursion_tabs(ss);
    ss << "class: " << (step->_class ? step->_class->fullName : "null") << endl;

    add_recursion_tabs(ss);
    ss << "function: " << (step->function ? step->function->fullName : "null") << endl;

    add_recursion_tabs(ss);
    ss << "integer: " << (step->integer) << endl;

    add_recursion_tabs(ss);
    ss << "decimal: " << (step->decimal) << endl;

    add_recursion_tabs(ss);
    ss << "char: " << (step->_char) << endl;

    add_recursion_tabs(ss);
    ss << "bool: " << (step->_bool) << endl;

    add_recursion_tabs(ss);
    ss << "nativeType: " << (step->nativeType) << endl;

    add_recursion_tabs(ss);
    ss << "string: " << (step->_string) << endl;

    add_recursion_tabs(ss);
    ss << "secondRegister: " << (step->secondRegister) << endl;

    add_recursion_tabs(ss);
    ss << "thirdRegister: " << (step->thirdRegister) << endl << endl;

    add_recursion_tabs(ss);
    ss << "scheme: " << endl;

    if(step->scheme != NULL) {
        ss << operation_scheme_to_str(step->scheme);
    } else {
        add_recursion_tabs(ss);
        ss << "null" << endl << endl;
    }

    space_count = oldSpaceCount;
}


string operation_scheme_to_str(operation_schema *schema) {
    stringstream ss;
    scheme_recursion++;
    add_recursion_tabs(ss);
    ss << "operation_schema @" << schema << ": ";

    switch(schema->schemeType) {
       case scheme_none:
           ss << "scheme_none";
           break;
       case scheme_access_primary_instance_field:
           ss << "scheme_access_primary_instance_field";
           break;
       case scheme_access_instance_field:
           ss << "scheme_access_instance_field";
           break;
       case scheme_access_static_field:
           ss << "scheme_access_static_field";
           break;
       case scheme_access_tls_field:
           ss << "scheme_access_tls_field";
           break;
       case scheme_negate_value:
           ss << "scheme_negate_value";
           break;
       case scheme_inc_value:
           ss << "scheme_inc_value";
           break;
       case scheme_dec_value:
           ss << "scheme_dec_value";
           break;
       case scheme_not_value:
           ss << "scheme_not_value";
           break;
       case scheme_master:
           ss << "scheme_master";
           break;
       case scheme_return:
           ss << "scheme_return";
           break;
       case scheme_get_constant:
           ss << "scheme_get_constant";
           break;
       case scheme_line_info:
           ss << "scheme_line_info";
           break;
       case scheme_call_primary_class_instance_function:
           ss << "scheme_call_primary_class_instance_function";
           break;
       case scheme_call_instance_function:
           ss << "scheme_call_instance_function";
           break;
       case scheme_call_dynamic_function:
           ss << "scheme_call_dynamic_function";
           break;
       case scheme_call_static_function:
           ss << "scheme_call_static_function";
           break;
       case scheme_get_primary_class_instance:
           ss << "scheme_get_primary_class_instance";
           break;
       case scheme_access_local_field:
           ss << "scheme_access_local_field";
           break;
       case scheme_new_class_array:
           ss << "scheme_new_class_array";
           break;
       case scheme_new_number_array:
           ss << "scheme_new_number_array";
           break;
       case scheme_new_object_array:
           ss << "scheme_new_object_array";
           break;
       case scheme_nullify_value:
           ss << "scheme_nullify_value";
           break;
       case scheme_new_class:
           ss << "scheme_new_class";
           break;
       case scheme_assign_array_value:
           ss << "scheme_assign_array_value";
           break;
       case scheme_get_size_of:
           ss << "scheme_get_size_of";
           break;
       case scheme_get_casted_value:
           ss << "scheme_get_casted_value";
           break;
       case scheme_check_type:
           ss << "scheme_check_type";
           break;
       case scheme_null_fallback:
           ss << "scheme_null_fallback";
           break;
       case scheme_inline_if:
           ss << "scheme_inline_if";
           break;
       case scheme_assign_value:
           ss << "scheme_assign_value";
           break;
       case scheme_compound_assign_value:
           ss << "scheme_compound_assign_value";
           break;
       case scheme_binary_math:
           ss << "scheme_binary_math";
           break;
       case scheme_accelerated_binary_math:
           ss << "scheme_accelerated_binary_math";
           break;
       case scheme_or:
           ss << "scheme_or";
           break;
       case scheme_and:
           ss << "scheme_and";
           break;
       case scheme_null_check:
           ss << "scheme_null_check";
           break;
       case scheme_instance_check:
           ss << "scheme_instance_check";
           break;
       case scheme_post_increment:
           ss << "scheme_post_increment";
           break;
       case scheme_unused_data:
           ss << "scheme_unused_data";
           break;
       case scheme_get_value:
           ss << "scheme_get_value";
           break;
       case scheme_for:
           ss << "scheme_for";
           break;
       case scheme_for_infinite:
           ss << "scheme_for_infinite";
           break;
       case scheme_for_cond:
           ss << "scheme_for_cond";
           break;
       case scheme_for_iter:
           ss << "scheme_for_iter";
           break;
       case scheme_lock_data:
           ss << "scheme_lock_data";
           break;
       case scheme_unlock_data:
           ss << "scheme_unlock_data";
           break;
       case scheme_if_single:
           ss << "scheme_if_single";
           break;
       case scheme_compound_if:
           ss << "scheme_compound_if";
           break;
       case scheme_increment_for_index_value:
           ss << "scheme_increment_for_index_value";
           break;
       case scheme_for_each:
           ss << "scheme_for_each";
           break;
       case scheme_elseif:
           ss << "scheme_elseif";
           break;
       case scheme_else:
           ss << "scheme_else";
           break;
       case scheme_for_each_position_check:
           ss << "scheme_for_each_position_check";
           break;
       case scheme_for_each_get_array_item:
           ss << "scheme_for_each_get_array_item";
           break;
       case scheme_when:
           ss << "scheme_when";
           break;
       case scheme_when_clause:
           ss << "scheme_when_clause";
           break;
       case scheme_when_else_clause:
           ss << "scheme_when_else_clause";
           break;
       case scheme_try:
           ss << "scheme_try";
           break;
       case scheme_catch_clause:
           ss << "scheme_catch_clause";
           break;
       case scheme_last_finally_clause:
           ss << "scheme_last_finally_clause";
           break;
       case scheme_finally_clause:
           ss << "scheme_finally_clause";
           break;
       case scheme_return_address_check_1:
           ss << "scheme_return_address_check_1";
           break;
       case scheme_return_address_check_2:
           ss << "scheme_return_address_check_2";
           break;
       case scheme_while:
           ss << "scheme_while";
           break;
       case scheme_do_while:
           ss << "scheme_do_while";
           break;
       case scheme_throw:
           ss << "scheme_throw";
           break;
       case scheme_break:
           ss << "scheme_break";
           break;
       case scheme_continue:
           ss << "scheme_continue";
           break;
       case scheme_lock:
           ss << "scheme_lock";
           break;
       case scheme_call_getter_function:
           ss << "scheme_call_getter_function";
           break;
       case scheme_get_address:
           ss << "scheme_get_address";
           break;
       case scheme_label:
           ss << "scheme_label";
    }

    ss << endl;
    add_recursion_tabs(ss);
    ss << "  ";
    ss << "field: " << (schema->field ? schema->field->fullName : "null") << endl;

    add_recursion_tabs(ss);
    ss << "  ";
    ss << "type: " << type_to_str(schema->type) << endl;

    add_recursion_tabs(ss);
    ss << "  ";
    ss << "fun: " << (schema->fun ? schema->fun->fullName : "null") << endl;

    add_recursion_tabs(ss);
    ss << "  ";
    ss << "class: " << (schema->sc ? schema->sc->fullName : "null") << endl << endl;

    add_recursion_tabs(ss);
    ss << "  steps:" << endl;
    for(Int i = 0; i < schema->steps.size(); i++) {
        operation_step_to_str(i, schema->steps.get(i), ss);
    }

    scheme_recursion--;
    return ss.str();
}


atomic<Int> uniqueRegisterIds = {0};
atomic<Int> uniqueLabelIds = {0};
void create_local_field_access_operation(
        operation_schema *scheme,
        sharp_field *localField) {
    if(scheme) {
        scheme->schemeType = scheme_access_local_field;
        scheme->field = localField;

        scheme->steps.add(new operation_step(
                operation_get_local_field_value, localField));
    }
}

void create_static_field_access_operation(
        operation_schema *scheme,
        sharp_field *staticField,
        bool resetState) {
    if(scheme) {
        if(resetState) scheme->free();
        scheme->schemeType = scheme_access_static_field;
        scheme->field = staticField;

        scheme->steps.add(new operation_step(
                operation_get_static_class_instance, staticField->owner));
        scheme->steps.add(new operation_step(
                operation_get_instance_field_value, staticField));
    }
}

void create_tls_field_access_operation(
        operation_schema *scheme,
        sharp_field *tlsField,
        bool resetState) {
    if(scheme) {
        if(resetState) scheme->free();
        scheme->schemeType = scheme_access_tls_field;
        scheme->field = tlsField;

        scheme->steps.add(new operation_step(
                operation_get_tls_field_value, tlsField));
    }
}

void create_primary_instance_field_access_operation(
        operation_schema *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_access_primary_instance_field;
        scheme->field = instanceField;

        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, instanceField->owner));
        scheme->steps.add(new operation_step(
                operation_get_instance_field_value, instanceField));
    }
}

void create_machine_instruction_operation(
        operation_schema *scheme,
        machine_instruction *instruction) {
    scheme->steps.add(new operation_step(
            operation_machine_instruction, instruction));
}

void add_scheme_operation(
        operation_schema *scheme,
        operation_schema *valueScheme) {
    scheme->steps.add(new operation_step(valueScheme));
}

void add_get_field_value_scheme_operation(
        operation_schema *scheme,
        operation_schema *valueScheme,
        bool resetState) {
    if(resetState) scheme->free();
    scheme->schemeType = scheme_get_field_value;
    scheme->steps.add(new operation_step(valueScheme, operation_get_field_value));
}

void create_primary_instance_field_getter_operation(
        operation_schema *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_call_getter_function;
        scheme->fun = instanceField->getter;

        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, instanceField->owner));
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack));
        scheme->steps.add(new operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_static_field_getter_operation(
        operation_schema *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_call_getter_function;
        scheme->fun = instanceField->getter;

        scheme->steps.add(new operation_step(
                operation_get_static_class_instance, instanceField->owner));
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack));
        scheme->steps.add(new operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_new_class_array_operation(
        operation_schema *scheme,
        operation_schema *arraySizeOperations,
        sharp_class *sc) {
    if(scheme) {
        scheme->schemeType = scheme_new_class_array;

        scheme->steps.add(new operation_step(
                arraySizeOperations, operation_get_size_value));

        scheme->steps.add(new operation_step(
                operation_create_class_array, sc));
    }
}

void create_cast_operation(
        operation_schema *scheme,
        sharp_type *cast_type) {
    if(scheme) {
        scheme->schemeType = scheme_get_casted_value;

        if(cast_type->type == type_class) {
            scheme->steps.add(new operation_step(
                    operation_cast_class, cast_type->_class));
        } else if(cast_type->type == type_int8) {
            scheme->steps.add(new operation_step(
                    operation_int8_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_int16) {
            scheme->steps.add(new operation_step(
                    operation_int16_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_int32) {
            scheme->steps.add(new operation_step(
                    operation_int32_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_int64) {
            scheme->steps.add(new operation_step(
                    operation_int64_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_uint8) {
            scheme->steps.add(new operation_step(
                    operation_uint8_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_uint16) {
            scheme->steps.add(new operation_step(
                    operation_uint16_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_uint32) {
            scheme->steps.add(new operation_step(
                    operation_uint32_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } else if(cast_type->type == type_uint64) {
            scheme->steps.add(new operation_step(
                    operation_uint64_cast, cast_type->isArray));
            scheme->steps.last()->integer = cast_type->type;
        } // ignore var and object casts
    }
}

void create_null_fallback_operation(
        operation_schema *scheme,
        operation_schema *nullScheme,
        operation_schema *fallbackScheme) {
    scheme->schemeType = scheme_null_fallback;

    scheme->steps.add(new operation_step(
            operation_get_value, nullScheme));
    scheme->steps.add(new operation_step(
            operation_get_value, fallbackScheme));
}

void create_is_operation(
        operation_schema *scheme,
        sharp_type *cast_type) {
    if(scheme) {
        scheme->schemeType = scheme_check_type;

        if(cast_type->type == type_class) {
            scheme->steps.add(new operation_step(
                    operation_is_class, cast_type->_class));
        } else if(cast_type->type == type_int8) {
            scheme->steps.add(new operation_step(
                    operation_is_int8));
        } else if(cast_type->type == type_int16) {
            scheme->steps.add(new operation_step(
                    operation_is_int16));
        } else if(cast_type->type == type_int32) {
            scheme->steps.add(new operation_step(
                    operation_is_int32));
        } else if(cast_type->type == type_int64) {
            scheme->steps.add(new operation_step(
                    operation_is_int64));
        } else if(cast_type->type == type_uint8) {
            scheme->steps.add(new operation_step(
                    operation_is_uint8));
        } else if(cast_type->type == type_uint16) {
            scheme->steps.add(new operation_step(
                    operation_is_uint16));
        } else if(cast_type->type == type_uint32) {
            scheme->steps.add(new operation_step(
                    operation_is_uint32));
        } else if(cast_type->type == type_uint64) {
            scheme->steps.add(new operation_step(
                    operation_is_uint64));
        } else if(cast_type->type == type_function_ptr
            || cast_type->type == type_var) {
            scheme->steps.add(new operation_step(
                    operation_is_var));
        }
    }
}

void create_sizeof_operation(
        operation_schema *scheme,
        operation_schema *valueOperation) {
    if(scheme) {
        scheme->schemeType = scheme_get_size_of;
        scheme->steps.add(new operation_step(
                operation_get_value, valueOperation));
        scheme->steps.add(new operation_step(
                operation_get_sizeof));
    }
}

void create_throw_operation(
        operation_schema *scheme,
        operation_schema *valueOperation) {
    if(scheme) {
        scheme->schemeType = scheme_throw;
        scheme->steps.add(new operation_step(
                operation_get_value, valueOperation));
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack));

        scheme->steps.add(new operation_step(
                operation_throw_exception));
    }
}

void create_new_class_operation(
        operation_schema *scheme,
        sharp_class *sc) {
    if(scheme) {
        scheme->schemeType = scheme_new_class;
        scheme->steps.add(new operation_step(
                operation_create_class, sc));
    }
}

void create_new_number_array_operation(
        operation_schema *scheme,
        operation_schema *arraySizeOperations,
        data_type nativeType) {
    if(scheme) {
        scheme->schemeType = scheme_new_number_array;

        scheme->steps.add(new operation_step(
                arraySizeOperations, operation_get_size_value));

        scheme->steps.add(new operation_step(
                operation_create_number_array, nativeType));
    }
}

void create_new_object_array_operation(
        operation_schema *scheme,
        operation_schema *arraySizeOperations) {
    if(scheme) {
        scheme->schemeType = scheme_new_object_array;

        scheme->steps.add(new operation_step(
                arraySizeOperations, operation_get_size_value));

        scheme->steps.add(new operation_step(operation_create_object_array));
    }
}

void create_null_value_operation(operation_schema *scheme) {
    if(scheme) {
        if(scheme->schemeType == scheme_none)
            scheme->schemeType = scheme_nullify_value;

        scheme->steps.add(new operation_step(
                operation_nullify_value));
    }
}

void create_push_to_stack_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack));
    }
}

void create_post_scheme_start_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_post_scheme_start));
    }
}

void create_check_null_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_check_null));
    }
}

void create_and_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_and, registerLeft, registerRight));
}

void create_xor_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_xor, registerLeft, registerRight));
}

void create_or_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_or, registerLeft, registerRight));
}

void create_and_and_operation(
        operation_schema *scheme,
        operation_schema *leftScheme,
        operation_schema *rightScheme) {
    scheme->schemeType = scheme_and;

    Int endLabel = create_allocate_label_operation(scheme);

    create_get_value_operation(scheme, leftScheme, false, false);
    create_jump_if_false_operation(scheme, endLabel); // && short circuit

    create_get_value_operation(scheme, rightScheme, false, false);
    create_jump_if_false_operation(scheme, endLabel); // && short circuit
    create_set_label_operation(scheme, endLabel);
}

void create_or_or_operation(
        operation_schema *scheme,
        operation_schema *leftScheme,
        operation_schema *rightScheme) {
    scheme->schemeType = scheme_or;

    Int endLabel = create_allocate_label_operation(scheme);

    create_get_value_operation(scheme, leftScheme, false, false);
    create_jump_if_true_operation(scheme, endLabel);

    create_get_value_operation(scheme, rightScheme, false, false);
    create_set_label_operation(scheme, endLabel);
}

void create_eq_eq_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_eq_eq, registerLeft, registerRight));
}

void create_not_eq_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_not_eq, registerLeft, registerRight));
}

void create_lt_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_lt, registerLeft, registerRight));
}

void create_gt_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_gt, registerLeft, registerRight));
}

void create_lte_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_lte, registerLeft, registerRight));
}

void create_shl_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_shl, registerLeft, registerRight));
}

void create_shr_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_shr, registerLeft, registerRight));
}

void create_add_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_add, registerLeft, registerRight));
}

void create_sub_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_sub, registerLeft, registerRight));
}

void create_div_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_div, registerLeft, registerRight));
}

void create_mod_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_mod, registerLeft, registerRight));
}

void create_exponent_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_exponent, registerLeft, registerRight));
}

void create_mult_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_mult, registerLeft, registerRight));
}

void create_gte_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight) {
    scheme->steps.add(new operation_step(
            operation_gte, registerLeft, registerRight));
}

void create_return_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->schemeType = scheme_return;

        scheme->steps.add(new operation_step(
                operation_return_nil));
    }
}

void create_return_with_error_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->schemeType = scheme_return;

        scheme->steps.add(new operation_step(
                operation_return_with_error_state));
    }
}


void create_numeric_return_operation(
        operation_schema *scheme,
        operation_schema *valueScheme) {
    if(scheme) {
        scheme->schemeType = scheme_return;

        scheme->steps.add(new operation_step(
                operation_return_number, valueScheme));
    }
}

void create_object_return_operation(
        operation_schema *scheme,
        operation_schema *valueScheme) {
    if(scheme) {
        scheme->schemeType = scheme_return;

        scheme->steps.add(new operation_step(
                operation_return_object, valueScheme));
    }
}

void create_lock_operation(
        operation_schema *scheme,
        operation_schema *lockScheme) {
    if(scheme) {
        scheme->schemeType = scheme_lock_data;
        scheme->steps.add(new operation_step(
                operation_lock, lockScheme));
    }
}

void create_unlock_operation(
        operation_schema *scheme,
        operation_schema *lockScheme) {
    if(scheme) {
        scheme->schemeType = scheme_unlock_data;
        scheme->steps.add(new operation_step(
                operation_unlock, lockScheme));
    }
}

void create_instance_eq_operation(
        operation_schema *scheme) {
    scheme->steps.add(new operation_step(
            operation_instance_eq));
}

void create_instance_not_eq_operation(
        operation_schema *scheme) {
    scheme->steps.add(new operation_step(
            operation_instance_not_eq));
}

void create_retain_numeric_value_operation(
        operation_schema *scheme,
        Int registerId) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_retain_numeric_value, registerId));
    }
}

void create_retain_label_value_operation(
        operation_schema *scheme,
        sharp_label* label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_retain_label_value, label->id));
    }
}

void create_branch_operation(
        operation_schema *scheme,
        Int registerId) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_branch, registerId));
    }
}

void create_deallocate_register_operation(
        operation_schema *scheme,
        Int registerId) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_discard_register, registerId));
    }
}

Int create_allocate_label_operation(
        operation_schema *scheme) {
        if(scheme) {
            scheme->steps.add(new operation_step(
                    operation_allocate_label, uniqueLabelIds++));
            return scheme->steps.last()->integer;
        }

        return -1;
}

Int create_allocate_try_catch_data_operation(
        operation_schema *scheme) {
        if(scheme) {
            scheme->steps.add(new operation_step(
                    operation_allocate_try_catch_data, uniqueLabelIds++));
            return scheme->steps.last()->integer;
        }

        return -1;
}

Int create_allocate_catch_data_operation(
        sharp_tc_data *parent,
        operation_schema *scheme) {
    if(scheme && parent) {
        scheme->steps.add(new operation_step(
                operation_allocate_catch_data, parent->id, uniqueLabelIds++));
        return scheme->steps.last()->secondRegister;
    }

    return -1;
}

Int create_allocate_finally_data_operation(
        sharp_tc_data *parent,
        operation_schema *scheme) {
    if (scheme && parent) {
        scheme->steps.add(new operation_step(
                operation_allocate_finally_data, parent->id, uniqueLabelIds++));
        return scheme->steps.last()->secondRegister;
    }

    return -1;
}


void create_set_catch_field_operation(
        catch_data_info *data,
        sharp_field *field,
        operation_schema *scheme) {
    if(scheme && data) {
        scheme->steps.add(new operation_step(
                operation_set_catch_field, data->parent->id, data->id));
        scheme->steps.last()->field = field;
    }
}

void create_set_finally_field_operation(
        finally_data_info *data,
        sharp_field *field,
        operation_schema *scheme) {
    if(scheme && data) {
        scheme->steps.add(new operation_step(
                operation_set_finally_exception_field, data->parent->id, data->id));
        scheme->steps.last()->field = field;
    }
}


void create_set_catch_class_operation(
        catch_data_info *data,
        sharp_class *sc,
        operation_schema *scheme) {
    if(scheme && data && sc) {
        scheme->steps.add(new operation_step(
                operation_set_catch_class, data->parent->id, data->id));
        scheme->steps.last()->_class = sc;
    }
}

void create_set_label_operation(
        operation_schema *scheme,
        Int label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_label, label));
    }
}

void create_set_label_operation(
        operation_schema *scheme,
        sharp_label* label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_label, label->id));
    }
}


void create_catch_start_operation(
        operation_schema *scheme,
        catch_data_info* data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_catch_start, data->parent->id, data->id));
    }
}

void create_finally_start_operation(
        operation_schema *scheme,
        finally_data_info* data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_finally_start, data->parent->id, data->id));
    }
}

void create_finally_end_operation(
        operation_schema *scheme,
        finally_data_info* data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_finally_end, data->parent->id, data->id));
    }
}

void create_try_catch_start_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_try_catch_start, tc_data->id));
    }
}

void create_try_catch_block_start_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_try_catch_block_start, tc_data->id));
    }
}

void create_try_catch_end_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_try_catch_end, tc_data->id));
    }
}

void create_no_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_no_op));
    }
}

void create_try_catch_block_end_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_try_catch_block_end, tc_data->id));
    }
}

void create_jump_if_false_operation(
        operation_schema *scheme,
        Int label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_jump_if_false, label));
    }
}

void create_jump_if_false_operation(
        operation_schema *scheme,
        sharp_label* label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_jump_if_false, label->id));
    }
}

void create_jump_if_true_operation(
        operation_schema *scheme,
        sharp_label* label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_jump_if_true, label->id));
    }
}

void create_jump_if_true_operation(
        operation_schema *scheme,
        Int label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_jump_if_true, label));
    }
}

void create_setup_local_field_operation(
        operation_schema *scheme,
        sharp_field* field) {
    if(scheme) {
        scheme->schemeType = scheme_access_local_field;
        scheme->field = field;

        scheme->steps.add(new operation_step(
                operation_setup_local_field, field));
    }
}

void create_jump_operation(
        operation_schema *scheme,
        sharp_label* label) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_jump, label->id));
    }
}

Int create_allocate_register_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_allocate_register, uniqueRegisterIds++));
        return scheme->steps.last()->integer;
    }

    return -1;
}

void create_unused_data_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_unused_data));
    }
}

void create_pop_value_from_stack_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_pop_value_from_stack));
    }
}

void create_pop_direct_object_value_from_stack_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_pop_direct_object_value_from_stack));
    }
}

void create_set_current_data_as_object_operation(
        operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_set_current_data_as_object));
    }
}

void create_pop_direct_numeric_value_from_stack_operation(
        operation_schema *scheme,
        Int registerId) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_pop_direct_numeric_value_from_stack, registerId));
    }
}

void create_assign_array_element_operation(
        operation_schema *scheme,
        Int index,
        bool isNumeric) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_assign_array_element_from_stack, index));
        scheme->steps.last()->_bool = isNumeric;
    }
}

void create_access_array_element_operation(
        operation_schema *scheme,
        sharp_type &type,
        operation_schema *indexScheme) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_get_array_element_at_index, indexScheme));
        scheme->steps.last()->nativeType = type.type;
    }
}

void create_instance_field_access_operation(
        operation_schema *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->schemeType = scheme_access_instance_field;
        scheme->field = instanceField;

        scheme->steps.add(new operation_step(
                operation_get_instance_field_value, instanceField));
    }
}

void create_instance_field_getter_operation(
        operation_schema *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->schemeType = scheme_call_getter_function;
        scheme->fun = instanceField->getter;

        scheme->steps.add(new operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_get_static_function_address_operation(
        operation_schema *scheme,
        sharp_function *fun,
        bool resetState) {
    if(scheme) {
        if(resetState)
            scheme->free();
        scheme->schemeType = scheme_get_address;

        scheme->steps.add(new operation_step(
                operation_get_static_function_address, fun));
    }
}

void create_primary_class_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun) {
    if(scheme) {

        scheme->schemeType = scheme_call_primary_class_instance_function;
        scheme->fun = fun;
        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, fun->owner
        ));

        scheme->steps.add(new operation_step(
                operation_push_value_to_stack
        ));

        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_schema(*paramScheme.get(i)),
                    operation_push_parameter_to_stack
                    ));
        }

        scheme->steps.add(new operation_step(
                operation_call_instance_function, fun));
    }
}

void create_instance_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun) {
    if(scheme) {
        scheme->schemeType = scheme_call_instance_function;
        scheme->fun = fun;
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack
        ));

        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_schema(*paramScheme.get(i)),
                    operation_push_parameter_to_stack
            ));
        }

        scheme->steps.add(new operation_step(
                operation_call_instance_function, fun));
    }
}

void create_get_integer_constant_operation(
        operation_schema *scheme,
        Int integer,
        bool resetState,
        bool setType) {
    if(setType)
        scheme->schemeType = scheme_get_constant;
    if(resetState)
        scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_integer_constant, integer));
}

void create_get_decimal_constant_operation(
        operation_schema *scheme,
        long double decimal,
        bool resetState,
        bool setType) {
    if(setType)
        scheme->schemeType = scheme_get_constant;
    if(resetState)
        scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_decimal_constant, decimal));
}

void create_line_record_operation(
        operation_schema *scheme,
        Int line) {
    scheme->schemeType = scheme_line_info;
    scheme->free();

    scheme->steps.add(new operation_step(
            operation_record_line, line));
}

void create_get_char_constant_operation(
        operation_schema *scheme,
        char _char,
        bool resetState,
        bool setType) {
    if(setType)
        scheme->schemeType = scheme_get_constant;
    if(resetState)
        scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_char_constant, _char));
}

void create_get_bool_constant_operation(
        operation_schema *scheme,
        bool _bool,
        bool resetState,
        bool setType) {
    if(setType)
        scheme->schemeType = scheme_get_constant;
    if(resetState)
        scheme->free();


    scheme->steps.add(new operation_step(
            operation_get_bool_constant, _bool));
}

void create_get_string_constant_operation(
        operation_schema *scheme,
        string &_string) {
    scheme->free();
    scheme->schemeType = scheme_get_constant;

    scheme->steps.add(new operation_step(
            operation_get_string_constant, _string));
}


void create_value_assignment_operation(
        operation_schema *scheme,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme,
        bool resetState) {
    if(resetState) scheme->free();
    scheme->schemeType = scheme_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_push_value_to_stack));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_pop_value_from_stack));

}

void create_array_index_assignment_operation(
        operation_schema *scheme,
        operation_schema *arrayScheme,
        operation_schema *valueScheme,
        bool resetState) {
    if(resetState) scheme->free();
    scheme->schemeType = scheme_assign_array_index;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_push_value_to_stack));

    scheme->steps.add(new operation_step(
            arrayScheme, operation_step_scheme));
}

void create_plus_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_add, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_sub_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_sub, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_mult_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_mult, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_div_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_div, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_mod_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_mod, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_and_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_and, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_or_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_or, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_xor_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme) {
    scheme->schemeType = scheme_compound_assign_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerRight));

    scheme->steps.add(new operation_step(
            asigneeScheme, operation_get_value));

    scheme->steps.add(new operation_step(
            operation_retain_numeric_value, registerLeft));

    scheme->steps.add(new operation_step(
            operation_xor, registerLeft, registerRight));

    scheme->steps.add(new operation_step(
            operation_assign_numeric_value));

}

void create_get_value_operation(
        operation_schema *scheme,
        operation_schema *valueScheme,
        bool resetState,
        bool setType) {
    if(resetState)
        scheme->free();
    if(setType)
        scheme->schemeType = scheme_get_value;

    scheme->steps.add(new operation_step(
            valueScheme, operation_get_value));
}

void create_post_access_operation(
        operation_schema *scheme) {
    scheme->steps.add(new operation_step(operation_post_access));
}

void create_negate_operation(operation_schema *scheme) {
    scheme->steps.add(new operation_step(operation_negate_value));
}
void create_not_operation(operation_schema *scheme) {
    scheme->steps.add(new operation_step(operation_not_value));
}

void create_increment_operation(
        operation_schema *scheme,
        data_type type) {
    if(type == type_var) {
        scheme->steps.add(new operation_step(operation_var_increment));
    } else if(type == type_int8) {
        scheme->steps.add(new operation_step(operation_int8_increment));
    } else if(type == type_int16) {
        scheme->steps.add(new operation_step(operation_int16_increment));
    } else if(type == type_int32) {
        scheme->steps.add(new operation_step(operation_int32_increment));
    } else if(type == type_int64) {
        scheme->steps.add(new operation_step(operation_int64_increment));
    } else if(type == type_uint8) {
        scheme->steps.add(new operation_step(operation_uint8_increment));
    } else if(type == type_uint16) {
        scheme->steps.add(new operation_step(operation_uint16_increment));
    } else if(type == type_uint32) {
        scheme->steps.add(new operation_step(operation_uint32_increment));
    } else if(type == type_uint64) {
        scheme->steps.add(new operation_step(operation_uint64_increment));
    } else {
        scheme->steps.add(new operation_step(operation_illegal));
    }
}

void create_duplicate_operation(operation_schema *scheme) {
    if(scheme) {
        scheme->steps.add(
                new operation_step(operation_duplicate_item)
        );
    }
}
void create_get_primary_instance_class(
        operation_schema *scheme,
        sharp_class *primaryClass) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, primaryClass));
    }
}

void create_decrement_operation(
        operation_schema *scheme,
        data_type type) {
    if(type == type_var) {
        scheme->steps.add(new operation_step(operation_var_decrement));
    } else if(type == type_int8) {
        scheme->steps.add(new operation_step(operation_int8_decrement));
    } else if(type == type_int16) {
        scheme->steps.add(new operation_step(operation_int16_decrement));
    } else if(type == type_int32) {
        scheme->steps.add(new operation_step(operation_int32_decrement));
    } else if(type == type_int64) {
        scheme->steps.add(new operation_step(operation_int64_decrement));
    } else if(type == type_uint8) {
        scheme->steps.add(new operation_step(operation_uint8_decrement));
    } else if(type == type_uint16) {
        scheme->steps.add(new operation_step(operation_uint16_decrement));
    } else if(type == type_uint32) {
        scheme->steps.add(new operation_step(operation_uint32_decrement));
    } else if(type == type_uint64) {
        scheme->steps.add(new operation_step(operation_uint64_decrement));
    } else {
        scheme->steps.add(new operation_step(operation_illegal));
    }
}

void create_static_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun,
        bool resetState) {
    if(scheme) {
        scheme->schemeType = scheme_call_static_function;
        scheme->fun = fun;
        if(resetState) scheme->free();

        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_schema(*paramScheme.get(i)),
                    operation_push_parameter_to_stack
            ));
        }

        scheme->steps.add(new operation_step(
                operation_call_static_function, fun));
    }
}

void create_dynamic_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun,
        Int registerId,
        bool resetState) {
    if(scheme) {
        scheme->schemeType = scheme_call_dynamic_function;

        if(resetState) scheme->free();

        create_retain_numeric_value_operation(scheme, registerId);
        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_schema(*paramScheme.get(i)),
                    operation_push_parameter_to_stack
            ));
        }

        scheme->steps.add(new operation_step(
                operation_call_dynamic_function, registerId));
        scheme->steps.last()->function = fun;
    }
}

void create_function_parameter_push_operation(
        sharp_type *paramType,
        Int matchResult,
        sharp_function *constructor,
        operation_schema *paramScheme,
        operation_schema *resultScheme) {
    if(paramType && paramScheme && resultScheme) {
        if(is_match_normal(matchResult)) {
            resultScheme->copy(*paramScheme);
        } else { // assuming matchResult is match_constructor
            sharp_class *with_class = paramType->_class;

            if(with_class) {
                resultScheme->schemeType = scheme_new_class;
                resultScheme->sc = with_class;

                resultScheme->steps.add(
                        new operation_step(
                            operation_create_class,
                            with_class
                        )
                );

                resultScheme->steps.add(
                        new operation_step(operation_duplicate_item)
                );

                List<operation_schema*> scheme;
                scheme.add(paramScheme);
                operation_schema *tmp = new operation_schema();
                create_instance_function_call_operation(
                        tmp, scheme, constructor);
                create_dependency(constructor);


                resultScheme->steps.add(
                        new operation_step(operation_step_scheme, tmp)
                );

                delete tmp;
                scheme.free();
            }
        }
    }
}

void operation_step::freeStep()  {
    if(scheme || instruction) {
        delete scheme;
        scheme = NULL;
        delete instruction;
        instruction = NULL;
    }
}

void operation_schema::appendSteps(const operation_schema &scheme){
    for(Int i = 0; i < scheme.steps.size(); i++) {
        steps.add(new operation_step(*scheme.steps.get(i)));
    }
}

void operation_schema::copy(const operation_schema &scheme) {
    schemeType = scheme.schemeType;
    field = scheme.field;
    fun = scheme.fun;
    sc = scheme.sc;
    type.copy(scheme.type);
    for(Int i = 0; i < scheme.steps.size(); i++) {
        steps.add(new operation_step(*scheme.steps.get(i)));
    }
}

void operation_schema::free() {
    deleteList(steps);
}
