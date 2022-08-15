//
// Created by bknun on 8/9/2022.
//

#include "step_processor.h"
#include "../../generator.h"
#include "../code_context.h"
#include "../../class_generator.h"
#include "../../field_generator.h"
#include "scheme_processor.h"

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
        case operation_get_tls_field_value:
            return process_get_tls_field_value(step);
        case operation_step_scheme:
            return process_scheme(step);
        case operation_record_line:
            return process_record_line_info(step);
        case operation_return_number:
            return process_return_number(step);
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
            return process_increment_value(step);
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

void process_get_tls_field_value(operation_step *step) {
    validate_step_type(step, operation_get_tls_field_value);

    sharp_field *tlsField = step->field;
    add_instruction(Opcode::Builder::tlsMovl(get_or_initialize_code(tlsField)->address));
    set_machine_data(tlsField, false, true);
}