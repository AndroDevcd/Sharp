//
// Created by bknun on 8/9/2022.
//

#include "step_processor.h"
#include "../../generator.h"
#include "../code_context.h"
#include "../../class_generator.h"
#include "../../field_generator.h"
#include "scheme_processor.h"
#include "../code_info.h"

void add_instruction(opcode_instr instr) {
    cc.instructions.add(instr);
}

void add_instruction(opcode_instr *instr) {
    for(int i = 0; i < INSTRUCTION_BUFFER_SIZE; i++) {
        cc.instructions.add(instr[i]);
    }
}

void validate_step_type(operation_step *step, operation_type type) {
    if(step->type != type) {
        stringstream ss;
        ss << "incorrect operation_step, found (" << step->type << ") when (" << type << ") was expected.";
        generation_error(ss.str());
    }
}

void process_step(operation_step *step) {
    switch (step->type) {
        case operation_get_static_class_instance:
            return process_get_static_class_instance(step);
        case operation_get_primary_class_instance:
            return process_get_primary_class_instance(step);
        case operation_get_instance_field_value:
            return process_get_instance_field_value(step);
        case operation_get_local_field_value:
            return process_get_local_field_value(step);
        case operation_get_tls_field_value:
            return process_get_tls_field_value(step);
        case operation_step_scheme:
            return process_scheme(step);
        case operation_record_line:
            return process_record_line_info(step);
        case operation_return_number:
            return process_return_number(step);
        case operation_return_object:
            return process_return_object(step);
        case operation_return_nil:
            return process_return_nil(step);
        case operation_get_value:
            return process_get_value(step);
        case operation_get_integer_constant:
            return process_get_integer_constant(step);
        case operation_get_bool_constant:
            return process_get_bool_constant(step);
        case operation_get_decimal_constant:
            return process_get_decimal_constant(step);
        case operation_not_value:
            return process_not_value(step);
        case operation_negate_value:
            return process_negate_value(step);
        case operation_var_increment:
        case operation_int8_increment:
        case operation_int16_increment:
        case operation_int32_increment:
        case operation_int64_increment:
        case operation_uint8_increment:
        case operation_uint16_increment:
        case operation_uint32_increment:
        case operation_uint64_increment:
            return process_increment_value(step);
        case operation_var_decrement:
        case operation_int8_decrement:
        case operation_int16_decrement:
        case operation_int32_decrement:
        case operation_int64_decrement:
        case operation_uint8_decrement:
        case operation_uint16_decrement:
        case operation_uint32_decrement:
        case operation_uint64_decrement:
            return process_decrement_value(step);
        case operation_call_static_function:
            return process_static_function_call(step);
        case operation_push_parameter_to_stack:
            return process_push_parameter_to_stack(step);
        case operation_create_class:
            return process_create_class(step);
        case operation_duplicate_item:
            return process_duplicate_item(step);
        case operation_push_value_to_stack:
            return process_push_value_to_stack(step);
        case operation_pop_value_from_stack:
            return process_pop_value_from_stack(step);
        case operation_allocate_register:
            return process_allocate_register(step);
        case operation_discard_register:
            return process_deallocate_register(step);
        case operation_retain_numeric_value:
            return process_retain_numeric_value(step);
        case operation_call_dynamic_function:
            return process_call_dynamic_function(step);
        case operation_call_instance_function:
            return process_instance_function_call(step);
        case operation_get_char_constant:
            return process_get_char_constant(step);
        case operation_get_string_constant:
            return process_get_string_constant(step);
        case operation_get_static_function_address:
            return process_get_static_function_address(step);
        case operation_nullify_value:
            return process_nullify_value(step);
        case operation_get_size_value:
            return process_get_size_value(step);
        case operation_create_class_array:
            return process_create_class_array(step);
        case operation_create_number_array:
            return process_create_number_array(step);
        case operation_create_object_array:
            return process_create_object_array(step);
        case operation_assign_array_element_from_stack:
            return process_assign_array_element_from_stack(step);
        case operation_assign_array_value:
            return process_assign_array_value(step);
        case operation_get_sizeof:
            return process_get_size_of(step);
        case operation_cast_class:
            return process_class_cast(step);
        case operation_int8_cast:
        case operation_int16_cast:
        case operation_int32_cast:
        case operation_int64_cast:
        case operation_uint8_cast:
        case operation_uint16_cast:
        case operation_uint32_cast:
        case operation_uint64_cast:
            return process_var_cast(step);
        case operation_is_class:
            return process_is_class(step);
        case operation_is_int8:
        case operation_is_int16:
        case operation_is_int32:
        case operation_is_int64:
        case operation_is_uint8:
        case operation_is_uint16:
        case operation_is_uint32:
        case operation_is_uint64:
            return process_is_type(step);
        case operation_jump_if_false:
            return process_jump_if_false(step);
        case operation_jump_if_true:
            return process_jump_if_true(step);
        case operation_branch:
            return process_branch(step);
        case operation_jump:
            return process_jump(step);
        case operation_set_label:
            return process_set_label(step);
        case operation_allocate_label:
            return process_allocate_label(step);
        case operation_add:
            return process_add(step);
        case operation_sub:
            return process_sub(step);
        case operation_mult:
            return process_mult(step);
        case operation_div:
            return process_div(step);
        case operation_mod:
            return process_mod(step);
        case operation_and:
            return process_and(step);
        case operation_or:
            return process_or(step);
        case operation_xor:
            return process_xor(step);
        case operation_assign_numeric_value:
            return process_assign_numeric_value(step);
        case operation_and_and:
            return process_and_and(step);
        case operation_eq_eq:
            return process_eq_eq(step);
        case operation_not_eq:
            return process_not_eq(step);
        case operation_lt:
            return process_lt(step);
        case operation_gt:
            return process_gt(step);
        case operation_gte:
            return process_gte(step);
        case operation_lte:
            return process_lte(step);
        case operation_shl:
            return process_shl(step);
        case operation_shr:
            return process_shr(step);
        case operation_exponent:
            return process_exponent(step);
        case operation_check_null:
            return process_check_null(step);
        case operation_instance_eq:
            return process_instance_eq(step);
        case operation_instance_not_eq:
            return process_instance_not_eq(step);
        case operation_post_access:
            return process_post_access(step);
        case operation_get_array_element_at_index:
            return process_get_array_element_at_index(step);
        case operation_unused_data:
            return process_unused_data(step);
        case operation_setup_local_field:
            return process_setup_local_field(step);
        case operation_unlock:
            return process_unlock(step);
        case operation_lock:
            return process_lock(step);
        case operation_no_op:
            return process_no_op(step);
        case operation_allocate_try_catch_data:
            return process_allocate_try_catch_data(step);
        case operation_set_try_catch_start:
            return process_set_try_catch_start(step);
        case operation_set_try_catch_block_start:
            return process_set_try_catch_block_start(step);
        case operation_set_try_catch_block_end:
            return process_set_try_catch_block_end(step);
        case operation_allocate_catch_data:
            return process_allocate_catch_data(step);
        case operation_set_catch_class:
            return process_set_catch_class(step);
        case operation_set_catch_field:
            return process_set_catch_field(step);
        case operation_set_catch_start:
            return process_set_catch_start(step);
        case operation_set_try_catch_end:
            return process_set_try_catch_end(step);
        case operation_allocate_finally_data:
            return process_allocate_finally_data(step);
        case operation_set_finally_start:
            return process_set_finally_start(step);
        case operation_set_finally_exception_field:
            return process_set_finally_exception_field(step);
        case operation_set_finally_end:
            return process_set_finally_end(step);
        case operation_return_with_error_state:
            return process_return_with_error_state(step);
        case operation_retain_label_value:
            return process_retain_label_value(step);
        default:
            generation_error("attempt to execute unknown operation step!");
            break;
    }
}

void process_retain_label_value(operation_step *step) {
    validate_step_type(step, operation_retain_label_value);

    create_dynamic_instruction(
            dynamic_instruction(Opcode::MOVI, INSTRUCTION_BUFFER_SIZE,
                                dynamic_argument(label_argument, step->integer),
                                dynamic_argument(regular_argument, ADX)
            )
    );

    set_machine_data(get_register(ADX));
}

void process_return_with_error_state(operation_step *step) {
    validate_step_type(step, operation_return_with_error_state);

    add_instruction(Opcode::Builder::ret(ERR_STATE));
}

void process_allocate_finally_data(operation_step *step) {
    validate_step_type(step, operation_allocate_finally_data);

    create_finally_data(step->secondRegister, step->integer);
}

void process_set_finally_exception_field(operation_step *step) {
    validate_step_type(step, operation_set_finally_exception_field);

    finally_data *fd =
            get_finally_data(step->secondRegister, find_try_catch_data(step->integer));

    if(fd != NULL) {
        fd->exceptionFieldAddress = step->field->ci->address;
    } else {
        generation_error("attempt to set unknown catch data!");
    }
}

void process_set_finally_end(operation_step *step) {
    validate_step_type(step, operation_set_finally_end);

    finally_data *fd =
            get_finally_data(step->secondRegister, find_try_catch_data(step->integer));

    if(fd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        fd->endPc = label;
    } else {
        generation_error("attempt to set unknown catch data!");
    }
}

void process_set_finally_start(operation_step *step) {
    validate_step_type(step, operation_set_finally_start);

    finally_data *fd =
            get_finally_data(step->secondRegister, find_try_catch_data(step->integer));

    if(fd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        fd->startPc = label;
    } else {
        generation_error("attempt to set unknown catch data!");
    }
}

void process_set_catch_start(operation_step *step) {
    validate_step_type(step, operation_set_catch_start);

    catch_data *cd =
            find_catch_data(step->secondRegister, find_try_catch_data(step->integer));

    if(cd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        cd->handlerPc = label;
    } else {
        generation_error("attempt to set unknown catch data!");
    }
}

void process_set_catch_field(operation_step *step) {
    validate_step_type(step, operation_set_catch_field);

    catch_data *cd =
            find_catch_data(step->secondRegister, find_try_catch_data(step->integer));

    if(cd != NULL) {
        if(step->field != NULL)
            cd->exceptionFieldAddress = step->field->ci->address;
        else
            cd->exceptionFieldAddress = -1;
    } else {
        generation_error("attempt to set unknown catch data!");
    }
}

void process_set_catch_class(operation_step *step) {
    validate_step_type(step, operation_set_catch_class);

    catch_data *cd =
            find_catch_data(step->secondRegister, find_try_catch_data(step->integer));

    if(cd != NULL) {
        cd->classAddress = step->_class->ci->address;
    } else {
        generation_error("attempt to set unknown catch data!");
    }
}

void process_allocate_catch_data(operation_step *step) {
    validate_step_type(step, operation_allocate_catch_data);

    create_catch_data(step->secondRegister, step->integer);
}

void process_set_try_catch_end(operation_step *step) {
    validate_step_type(step, operation_set_try_catch_end);

    try_catch_data *tcd =
            find_try_catch_data(step->integer);

    if(tcd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        tcd->tryEndPc = label;
    } else {
        generation_error("attempt to set unknown try catch data!");
    }
}

void process_set_try_catch_block_end(operation_step *step) {
    validate_step_type(step, operation_set_try_catch_block_end);

    try_catch_data *tcd =
            find_try_catch_data(step->integer);

    if(tcd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        tcd->blockEndPc = label;
    } else {
        generation_error("attempt to set unknown try catch data!");
    }
}

void process_set_try_catch_block_start(operation_step *step) {
    validate_step_type(step, operation_set_try_catch_block_start);

    try_catch_data *tcd =
            find_try_catch_data(step->integer);

    if(tcd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        tcd->blockStartPc = label;
    } else {
        generation_error("attempt to set unknown try catch data!");
    }
}

void process_set_try_catch_start(operation_step *step) {
    validate_step_type(step, operation_set_try_catch_start);

    try_catch_data *tcd =
            find_try_catch_data(step->integer);

    if(tcd != NULL) {
        Int label = create_label();
        set_label(find_label(label));
        tcd->tryStartPc = label;
    } else {
        generation_error("attempt to set unknown try catch data!");
    }
}

void process_allocate_try_catch_data(operation_step *step) {
    validate_step_type(step, operation_allocate_try_catch_data);

    create_try_catch_data(step->integer);
}

void process_no_op(operation_step *step) {
    validate_step_type(step, operation_no_op);

    add_instruction(Opcode::Builder::nop());
}

void process_lock(operation_step *step) {
    validate_step_type(step, operation_lock);

    process_scheme(step->scheme);
    consume_machine_data();

    add_instruction(Opcode::Builder::lock());
}

void process_unlock(operation_step *step) {
    validate_step_type(step, operation_unlock);

    process_scheme(step->scheme);
    consume_machine_data();

    add_instruction(Opcode::Builder::unlock());
}

void process_setup_local_field(operation_step *step) {
    validate_step_type(step, operation_setup_local_field);

    if(is_numeric_type(step->field->type) && !step->field->type.isArray) {
        add_instruction(Opcode::Builder::istorel(step->field->ci->address, 0));
    } else {
        add_instruction(Opcode::Builder::pushNull());
        add_instruction(Opcode::Builder::popl(step->field->ci->address));
    }
}

void process_get_array_element_at_index(operation_step *step) {
    validate_step_type(step, operation_get_array_element_at_index);

    push_machine_data_to_stack();
    process_scheme(step->scheme);
    consume_machine_data(get_register(ADX));

    add_instruction(Opcode::Builder::popObject2());
    add_instruction(Opcode::Builder::checklen(ADX));
    add_instruction(Opcode::Builder::movnd(ADX));

    if(step->nativeType <= type_var) {
        add_instruction(Opcode::Builder::iload(EBX));
        set_machine_data(get_register(EBX));
    } else {
        set_machine_data(generic_object_data);
    }
}

void process_post_access(operation_step *step) {
    validate_step_type(step, operation_post_access);

    if(cc.machineData.type == numeric_register_data
        || cc.machineData.type == numeric_instance_field
        || cc.machineData.type == numeric_local_field
        || cc.machineData.type == function_numeric_data) {
        consume_machine_data(get_register(EBX));
        set_machine_data(get_register(EBX));
    } else {
        consume_machine_data();
        set_machine_data(generic_object_data);
    }
}

void process_unused_data(operation_step *step) {
    validate_step_type(step, operation_unused_data);

    if(cc.machineData.type == string_constant
        || cc.machineData.type == new_class_data
        || cc.machineData.type == function_numeric_data
        || cc.machineData.type == function_object_data) {
        add_instruction(Opcode::Builder::pop());
    }

    clear_machine_data();
}

void process_instance_eq(operation_step *step) {
    validate_step_type(step, operation_instance_eq);

    add_instruction(Opcode::Builder::itest(CMT));
    set_machine_data(get_register(CMT));
}

void process_instance_not_eq(operation_step *step) {
    validate_step_type(step, operation_instance_not_eq);

    add_instruction(Opcode::Builder::itest(CMT));
    add_instruction(Opcode::Builder::_not(CMT, CMT));
    set_machine_data(get_register(CMT));
}

void process_check_null(operation_step *step) {
    validate_step_type(step, operation_check_null);

    add_instruction(Opcode::Builder::checkNull(CMT));
    set_machine_data(get_register(CMT));
}

void process_exponent(operation_step *step) {
    validate_step_type(step, operation_exponent);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::exp(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_shr(operation_step *step) {
    validate_step_type(step, operation_shr);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::shr(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_shl(operation_step *step) {
    validate_step_type(step, operation_shl);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::shl(
           BMR,  (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_gte(operation_step *step) {
    validate_step_type(step, operation_gte);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::ge(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(CMT));
}

void process_lte(operation_step *step) {
    validate_step_type(step, operation_lte);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::le(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(CMT));
}

void process_gt(operation_step *step) {
    validate_step_type(step, operation_gt);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::gt(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(CMT));
}

void process_lt(operation_step *step) {
    validate_step_type(step, operation_lt);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::lt(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(CMT));
}

void process_not_eq(operation_step *step) {
    validate_step_type(step, operation_not_eq);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::tne(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_eq_eq(operation_step *step) {
    validate_step_type(step, operation_eq_eq);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::te(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_and_and(operation_step *step) {
    validate_step_type(step, operation_and_and);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::_and(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_assign_numeric_value(operation_step *step) {
    validate_step_type(step, operation_assign_numeric_value);

    consume_machine_data_from_register(get_register(BMR));
    set_machine_data(get_register(BMR));
}

internal_register* get_register_or_consume(Int registerId, bool leftSide) {
    internal_register* r = find_register(registerId);

    if(r->type == normal_register)
        return r;
    else {
        add_instruction(Opcode::Builder::loadl(leftSide ? EGX : EBX, r->address));
        return get_register(leftSide ? EGX : EBX);
    }
}

void process_add(operation_step *step) {
    validate_step_type(step, operation_add);

    internal_register* left = step->integer == -1 ?
            get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::add(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_sub(operation_step *step) {
    validate_step_type(step, operation_sub);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::sub(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_mult(operation_step *step) {
    validate_step_type(step, operation_mult);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::mul(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_div(operation_step *step) {
    validate_step_type(step, operation_div);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::div(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_mod(operation_step *step) {
    validate_step_type(step, operation_mod);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::mod(
            BMR, (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_and(operation_step *step) {
    validate_step_type(step, operation_and);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::uand(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_or(operation_step *step) {
    validate_step_type(step, operation_or);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::_or(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_xor(operation_step *step) {
    validate_step_type(step, operation_xor);

    internal_register* left = step->integer == -1 ?
                              get_register(BMR) : get_register_or_consume(step->integer, true);
    internal_register* right = get_register_or_consume( step->secondRegister, false);

    add_instruction(Opcode::Builder::_xor(
            (_register) left->address, (_register)right->address));
    set_machine_data(get_register(BMR));
}

void process_jump_if_false(operation_step *step) {
    validate_step_type(step, operation_jump_if_false);

    consume_machine_data(get_register(CMT));
    create_dynamic_instruction(
            dynamic_instruction(Opcode::JNE, 1,
                                dynamic_argument(label_argument, step->integer)
            )
    );
}

void process_jump_if_true(operation_step *step) {
    validate_step_type(step, operation_jump_if_true);

    consume_machine_data(get_register(CMT));
    create_dynamic_instruction(
            dynamic_instruction(Opcode::JE, 1,
                                dynamic_argument(label_argument, step->integer)
            )
    );
}

void process_branch(operation_step *step) {
    validate_step_type(step, operation_branch);

    set_machine_data(find_register(step->integer));
    consume_machine_data(get_register(ADX));
    add_instruction(Opcode::Builder::brh());
}

void process_jump(operation_step *step) {
    validate_step_type(step, operation_jump);

    create_dynamic_instruction(
            dynamic_instruction(Opcode::JMP, 1,
                                dynamic_argument(label_argument, step->integer)
            )
    );
}

void process_set_label(operation_step *step) {
    validate_step_type(step, operation_set_label);

    set_label(find_label(step->integer));
}

void process_allocate_label(operation_step *step) {
    validate_step_type(step, operation_allocate_label);

    create_label(step->integer);
}

void process_is_type(operation_step *step) {
    data_type type = type_undefined;
    if(step->type == operation_is_var) {
        type = type_var;
    } else if(step->type == operation_is_int8) {
        type = type_int8;
    } else if(step->type == operation_is_int16) {
        type = type_int16;
    } else if(step->type == operation_is_int32) {
        type = type_int32;
    } else if(step->type == operation_is_int64) {
        type = type_int64;
    } else if(step->type == operation_is_uint8) {
        type = type_uint8;
    } else if(step->type == operation_is_uint16) {
        type = type_uint16;
    } else if(step->type == operation_is_uint32) {
        type = type_uint32;
    } else if(step->type == operation_is_uint64) {
        type = type_uint64;
    } else {
        generation_error("incorrect data type found for type check!");
    }

    decrement_machine_data(type);
    add_instruction(Opcode::Builder::is(BMR, type));
    set_machine_data(get_register(BMR));
}

void process_is_class(operation_step *step) {
    validate_step_type(step, operation_is_class);

    consume_machine_data();
    add_instruction(Opcode::Builder::is(BMR, step->_class->ci->address));
    set_machine_data(get_register(BMR));
}

void process_var_cast(operation_step *step) {
    if(step->type >= operation_int8_cast && step->type <=operation_uint64_cast) {
        consume_machine_data();
        add_instruction(Opcode::Builder::varCast(step->integer, step->_bool));
        set_machine_data(generic_object_data);
    }
    else {
        generation_error("incorrect numeric cast operation type found!");
    }
}

void process_class_cast(operation_step *step) {
    validate_step_type(step, operation_cast_class);

    consume_machine_data();
    add_instruction(Opcode::Builder::cast(step->_class->ci->address));
    set_machine_data(generic_object_data);
}

void process_assign_array_element_from_stack(operation_step *step) {
    validate_step_type(step, operation_assign_array_element_from_stack);

    add_instruction(Opcode::Builder::movsl(-1));
    add_instruction(Opcode::Builder::movn(step->integer));

    if(!step->_bool) {
        set_machine_data(array_object_data);
        add_instruction(Opcode::Builder::popObject());
    } else {
        set_machine_data(array_numeric_data);
        add_instruction(Opcode::Builder::loadValue(EGX));
        add_instruction(Opcode::Builder::imov(EGX));
    }
}

void process_get_size_of(operation_step *step) {
    validate_step_type(step, operation_get_sizeof);

    consume_machine_data();
    add_instruction(Opcode::Builder::_sizeof(EBX));
    set_machine_data(get_register(EBX));
}

void process_assign_array_value(operation_step *step) {
    validate_step_type(step, operation_assign_array_value);

    process_scheme(step->scheme);
}

void process_create_number_array(operation_step *step) {
    validate_step_type(step, operation_create_number_array);

    add_instruction(Opcode::Builder::newVarArray(ADX, step->nativeType));
    set_machine_data(array_numeric_data);
}

void process_create_object_array(operation_step *step) {
    validate_step_type(step, operation_create_object_array);

    add_instruction(Opcode::Builder::newObjectArray(ADX));
    set_machine_data(array_object_data);
}

void process_create_class_array(operation_step *step) {
    validate_step_type(step, operation_create_class_array);

    add_instruction(Opcode::Builder::newClassArray(ADX, step->_class->ci->address));
    set_machine_data(array_object_data);
}

void process_get_size_value(operation_step *step) {
    validate_step_type(step, operation_get_size_value);

    process_scheme(step->scheme);
    consume_machine_data(get_register(ADX));
}

void process_nullify_value(operation_step *step) {
    validate_step_type(step, operation_nullify_value);

    set_machine_data(null_data);
}

void process_get_static_function_address(operation_step *step) {
    validate_step_type(step, operation_get_static_function_address);

    Int address = create_constant(step->function->ci->address);
    set_machine_data(numeric_constant, address);
}

void process_get_string_constant(operation_step *step) {
    validate_step_type(step, operation_get_string_constant);

    Int address = create_constant(step->_string);
    add_instruction(Opcode::Builder::newString(address));
    set_machine_data(string_constant);
}

void process_get_char_constant(operation_step *step) {
    validate_step_type(step, operation_get_char_constant);

    Int address = create_constant(step->_char);
    set_machine_data(numeric_constant, address);
}

void process_retain_numeric_value(operation_step *step) {
    validate_step_type(step, operation_retain_numeric_value);

    auto reg = find_register(step->integer);
    consume_machine_data(reg);
}

void process_deallocate_register(operation_step *step) {
    validate_step_type(step, operation_discard_register);

    release_register(find_register(step->integer));
}

void process_allocate_register(operation_step *step) {
    validate_step_type(step, operation_allocate_register);

    allocate_register(step->integer);
}

void process_duplicate_item(operation_step *step) {
    validate_step_type(step, operation_duplicate_item);

    add_instruction(Opcode::Builder::dup());
}

void process_create_class(operation_step *step) {
    validate_step_type(step, operation_create_class);

    add_instruction(Opcode::Builder::newClass(step->_class->ci->address));
    set_machine_data(new_class_data);
}

void process_push_value_to_stack(operation_step *step) {
    validate_step_type(step, operation_push_value_to_stack);

    push_machine_data_to_stack();
}

void process_pop_value_from_stack(operation_step *step) {
    validate_step_type(step, operation_pop_value_from_stack);

    pop_machine_data_from_stack();
}

void process_push_parameter_to_stack(operation_step *step) {
    validate_step_type(step, operation_push_parameter_to_stack);

    process_scheme(step->scheme);
    push_machine_data_to_stack();
}

void process_instance_function_call(operation_step *step) {
    validate_step_type(step, operation_call_instance_function);

    add_instruction(Opcode::Builder::call(step->function->ci->address));

    if(is_numeric_type(step->function->returnType) && !step->function->returnType.isArray) {
        set_machine_data(function_numeric_data);
    } else if(step->function->returnType.type != type_nil){
        set_machine_data(function_object_data);
    }
}

void process_call_dynamic_function(operation_step *step) {
    validate_step_type(step, operation_call_dynamic_function);

    auto reg = find_register(step->integer);

    if(reg != NULL) {
        Int address = reg->address;
        if(reg->address != normal_register) {
            set_machine_data(reg);
            consume_machine_data(get_register(EGX));
            address = EGX;
        }

        add_instruction(Opcode::Builder::calld((_register) address));

        if (is_numeric_type(step->function->returnType) && !step->function->returnType.isArray) {
            set_machine_data(function_numeric_data);
        } else if (step->function->returnType.type != type_nil) {
            set_machine_data(function_object_data);
        }
    } else {
        generation_error("attempt to call dynamic function on unknown register!");
    }
}

void process_static_function_call(operation_step *step) {
    validate_step_type(step, operation_call_static_function);

    add_instruction(Opcode::Builder::call(step->function->ci->address));

    if(is_numeric_type(step->function->returnType) && !step->function->returnType.isArray) {
        set_machine_data(function_numeric_data);
    } else if(step->function->returnType.type != type_nil){
        set_machine_data(function_object_data);
    }
}

void process_get_value(operation_step *step) {
    validate_step_type(step, operation_get_value);

    process_scheme(step->scheme);
}

void process_decrement_value(operation_step *step) {
    data_type type = type_undefined;
    if(step->type == operation_var_decrement) {
        type = type_var;
    } else if(step->type == operation_int8_decrement) {
        type = type_int8;
    } else if(step->type == operation_int16_decrement) {
        type = type_int16;
    } else if(step->type == operation_int32_decrement) {
        type = type_int32;
    } else if(step->type == operation_int64_decrement) {
        type = type_int64;
    } else if(step->type == operation_uint8_decrement) {
        type = type_uint8;
    } else if(step->type == operation_uint16_decrement) {
        type = type_uint16;
    } else if(step->type == operation_uint32_decrement) {
        type = type_uint32;
    } else if(step->type == operation_uint64_decrement) {
        type = type_uint64;
    } else {
        generation_error("incorrect data type found for increment!");
    }

    decrement_machine_data(type);
}

void process_increment_value(operation_step *step) {
    data_type type = type_undefined;
    if(step->type == operation_var_increment) {
        type = type_var;
    } else if(step->type == operation_int8_increment) {
        type = type_int8;
    } else if(step->type == operation_int16_increment) {
        type = type_int16;
    } else if(step->type == operation_int32_increment) {
        type = type_int32;
    } else if(step->type == operation_int64_increment) {
        type = type_int64;
    } else if(step->type == operation_uint8_increment) {
        type = type_uint8;
    } else if(step->type == operation_uint16_increment) {
        type = type_uint16;
    } else if(step->type == operation_uint32_increment) {
        type = type_uint32;
    } else if(step->type == operation_uint64_increment) {
        type = type_uint64;
    } else {
        generation_error("incorrect data type found for increment!");
    }

    increment_machine_data(type);
}

void process_negate_value(operation_step *step) {
    validate_step_type(step, operation_negate_value);

    consume_machine_data(get_register(EBX));
    add_instruction(Opcode::Builder::neg(EBX, EBX));
    set_machine_data(get_register(EBX));
}

void process_not_value(operation_step *step) {
    validate_step_type(step, operation_not_value);

    consume_machine_data(get_register(EBX));
    add_instruction(Opcode::Builder::_not(EBX, EBX));
    set_machine_data(get_register(EBX));
}

void process_get_decimal_constant(operation_step *step) {
    validate_step_type(step, operation_get_decimal_constant);

    Int address = create_constant(step->decimal);
    set_machine_data(numeric_constant, address);
}

void process_get_bool_constant(operation_step *step) {
    validate_step_type(step, operation_get_bool_constant);

    Int address = create_constant(step->_bool ? 1 : 0);
    set_machine_data(numeric_constant, address);
}

void process_get_integer_constant(operation_step *step) {
    validate_step_type(step, operation_get_integer_constant);

    Int address = create_constant(step->integer);
    set_machine_data(numeric_constant, address);
}

void process_return_nil(operation_step *step) {
    validate_step_type(step, operation_return_nil);

    add_instruction(Opcode::Builder::ret(NO_ERR));
}

void process_return_object(operation_step *step) {
    validate_step_type(step, operation_return_object);

    process_scheme(step->scheme);

    consume_machine_data();
    add_instruction(Opcode::Builder::returnObject());
    add_instruction(Opcode::Builder::ret(NO_ERR));
}

void process_return_number(operation_step *step) {
    validate_step_type(step, operation_return_number);

    process_scheme(step->scheme);

    _register r = EBX;
    if(cc.machineData.type == numeric_register_data) {
        r = (_register)cc.machineData.dataAddress;
        cc.machineData.type = no_data;
    } else {
        consume_machine_data(get_register(EBX));
    }

    add_instruction(Opcode::Builder::returnValue(r));
    add_instruction(Opcode::Builder::ret(NO_ERR));
}

void process_record_line_info(operation_step *step) {
    validate_step_type(step, operation_record_line);

    cc.lineTable.add(new line_info(cc.instructions.size(), step->integer));
}

void process_scheme(operation_step *step) {
    validate_step_type(step, operation_step_scheme);

    process_scheme(step->scheme);
}

void process_get_static_class_instance(operation_step *step) {
    validate_step_type(step, operation_get_static_class_instance);

    sharp_class *staticClass = step->_class;
    add_instruction(Opcode::Builder::movg(get_or_initialize_code(staticClass)->address));
    set_machine_data(class_object_data, get_or_initialize_code(staticClass)->address, true);
}

void process_get_primary_class_instance(operation_step *step) {
    validate_step_type(step, operation_get_primary_class_instance);

    add_instruction(Opcode::Builder::movl(0));
    set_machine_data(class_object_data, 0, true);
}

void process_get_instance_field_value(operation_step *step) {
    validate_step_type(step, operation_get_instance_field_value);

    sharp_field *staticField = step->field;
    add_instruction(Opcode::Builder::movn(get_or_initialize_code(staticField)->address));
    set_machine_data(staticField, false, true);
}

void process_get_local_field_value(operation_step *step) {
    validate_step_type(step, operation_get_local_field_value);

    sharp_field *localField = step->field;
    set_machine_data(localField, true, false);
}

void process_get_tls_field_value(operation_step *step) {
    validate_step_type(step, operation_get_tls_field_value);

    sharp_field *tlsField = step->field;
    add_instruction(Opcode::Builder::tlsMovl(get_or_initialize_code(tlsField)->address));
    set_machine_data(tlsField, false, true);
}