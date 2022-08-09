//
// Created by bknun on 8/9/2022.
//

#include "step_processor.h"
#include "../../generator.h"
#include "../code_context.h"
#include "../../class_generator.h"
#include "../../field_generator.h"

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

void process_get_static_class_instance(operation_step *step) {
    validate_step_type(step, operation_get_static_class_instance);

    sharp_class *staticClass = step->_class;
    add_instruction(Opcode::Builder::movg(get_or_initialize_code(staticClass)->address));
}

void process_get_primary_class_instance(operation_step *step) {
    validate_step_type(step, operation_get_primary_class_instance);

    add_instruction(Opcode::Builder::movl(0));
}

void process_get_instance_field_value(operation_step *step) {
    validate_step_type(step, operation_get_instance_field_value);

    sharp_field *staticField = step->field;
    add_instruction(Opcode::Builder::movn(get_or_initialize_code(staticField)->address));
}