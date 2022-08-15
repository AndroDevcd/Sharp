//
// Created by bknun on 8/9/2022.
//

#include "code_context.h"
#include "code_info.h"
#include "../generator.h"
#include "scheme/step_processor.h"

code_context cc;

void code_context::free() {
    deleteList(lineTable);
    deleteList(registers);
    instructions.free();
    container = NULL;
    ci = NULL;
    cc.machineData.type = no_data;
}

void update_context(code_info* ci, sharp_function *container) {
    cc.ci = ci;
    cc.container = container;
    setup_register_alloc();
}

void flush_context() {
    for(Int i = 0; i < cc.lineTable.size(); i++) {
        cc.ci->lineTable.add(new line_info(*cc.lineTable.get(i)));
    }

    cc.ci->stackSize = (check_flag(cc.container->flags, flag_static) ? 0 : 1) +
            cc.container->locals.size() + (cc.registers.size() - MIN_REGISTERS);
    cc.ci->code.appendAll(cc.instructions);
    cc.free();
}

void set_machine_data(sharp_field *field, bool localField, bool force) {
    if(is_numeric_type(field->type) && !field->type.isArray) {
        set_machine_data(localField ? numeric_local_field : numeric_instance_field,
                         field->ci->address, force);
    } else {
        set_machine_data(localField ? local_field_object_data : field_object_data,
                         field->ci->address, force);
    }
}

void set_machine_data(internal_register *internalRegister, bool force) {
    if(internalRegister->type == normal_register) {
        set_machine_data(numeric_register_data, internalRegister->address, force);
    } else {
        set_machine_data(numeric_local_field, internalRegister->address, force);
    }
}

void set_machine_data(machine_data_type dataType, Int dataAddr, bool force) {
    if(cc.machineData.type == no_data || force) {
        cc.machineData.type = dataType;
        cc.machineData.dataAddress = dataAddr;
    } else {
        generation_error("previous machine data not consumed!");
    }
}

void set_machine_data(machine_data_type dataType) {
    if(cc.machineData.type == no_data) {
        cc.machineData.type = dataType;
        cc.machineData.dataAddress = -1;
    } else {
        generation_error("previous machine data not consumed!");
    }
}

void consume_machine_data() {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case numeric_constant: {
                generation_error("attempting to consume numeric data instead of object!");
                break;
            }
            case string_constant: {
                add_instruction(Opcode::Builder::newString(cc.machineData.dataAddress));
                // todo: would we want to pull this off the stack after creation?
                break;
            }
            case local_field_object_data: {
                add_instruction(Opcode::Builder::movl(cc.machineData.dataAddress));
                break;
            }

            case numeric_register_data: {
                generation_error("attempting to consume numeric register data instead of object!");
                break;
            }

            case function_numeric_data:
            case numeric_instance_field:
            case numeric_local_field: {
                generation_error("attempting to consume numeric data instead of object!");
                break;
            }

            case generic_object_data:
            case new_class_data:
            case field_object_data: {
                // do nothing
                cc.machineData.dataAddress = -1;
                break;
            }

            case function_object_data:
            case class_object_data: {
                add_instruction(Opcode::Builder::popObject2());
                break;
            }
        }

        cc.machineData.type = no_data;
    } else {
        generation_error("attempting to consume empty machine data!");
    }
}

void push_machine_data_to_stack() {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case numeric_constant: {
                if (floor(constantMap.get(cc.machineData.dataAddress)) == constantMap.get(cc.machineData.dataAddress)) {
                    add_instruction(Opcode::Builder::istore( // todo: check if number is < int32 max
                            (opcode_arg)constantMap.get(cc.machineData.dataAddress)));
                } else {
                    add_instruction(Opcode::Builder::ldc(
                            EGX, cc.machineData.dataAddress));
                    add_instruction(Opcode::Builder::rstore(EGX));
                }
                break;
            }
            case string_constant: {
                add_instruction(Opcode::Builder::newString(cc.machineData.dataAddress));
                break;
            }
            case numeric_local_field: {
                add_instruction(Opcode::Builder::ipushl(
                        cc.machineData.dataAddress));
                break;
            }
            case numeric_register_data: {
                add_instruction(Opcode::Builder::rstore((_register) cc.machineData.dataAddress));
                break;
            }
            case numeric_instance_field: {
                consume_machine_data(get_register(EGX));
                add_instruction(Opcode::Builder::rstore(EGX));
                break;
            }
            case class_object_data:
            case generic_object_data:
            case local_field_object_data:
            case field_object_data: {
                add_instruction(Opcode::Builder::pushObject());
                break;
            }

            case function_object_data:
            case function_numeric_data:
            case new_class_data: {
                // do nothing
                cc.machineData.dataAddress = -1;
            }
        }

        cc.machineData.type = no_data;
    } else {
        generation_error("attempting to consume empty machine data!");
    }
}

void consume_machine_data(internal_register *internalRegister) {
    if(internalRegister != NULL) {
        if (cc.machineData.type != no_data) {
            switch (cc.machineData.type) {
                case no_data:
                    break;
                case numeric_constant: {
                    if (internalRegister->type == normal_register) {
                        add_instruction(Opcode::Builder::ldc(
                                (_register) internalRegister->address, cc.machineData.dataAddress));
                    } else {
                        add_instruction(Opcode::Builder::ldc(EGX, cc.machineData.dataAddress));
                        add_instruction(Opcode::Builder::smovr2(EGX, internalRegister->address));
                    }
                    break;
                }
                case string_constant: {
                    generation_error("attempting to consume string data into register!");
                    break;
                }
                case numeric_local_field: {
                    if (internalRegister->type == normal_register) {
                        add_instruction(Opcode::Builder::loadl(
                                (_register) internalRegister->address, cc.machineData.dataAddress));
                    } else {
                        add_instruction(Opcode::Builder::smovr4(
                                internalRegister->address, cc.machineData.dataAddress));
                    }
                    break;
                }
                case numeric_register_data: {

                    if (internalRegister->type == normal_register) {
                        if (internalRegister->address != cc.machineData.dataAddress) {
                            add_instruction(Opcode::Builder::movr(
                                    (_register) internalRegister->address, (_register) cc.machineData.dataAddress));
                        }
                    } else {
                        add_instruction(Opcode::Builder::smovr2(
                                (_register) cc.machineData.dataAddress, internalRegister->address));
                    }
                    break;
                }
                case numeric_instance_field: {
                    if (internalRegister->type == normal_register) {
                        add_instruction(Opcode::Builder::iload((_register) internalRegister->address));
                    } else {
                        add_instruction(Opcode::Builder::iload(EGX));
                        add_instruction(Opcode::Builder::smovr2(EGX, internalRegister->address));
                    }
                    break;
                }
                case function_numeric_data: {
                    if (internalRegister->type == normal_register) {
                        add_instruction(Opcode::Builder::loadValue((_register) internalRegister->address));
                    } else {
                        add_instruction(Opcode::Builder::loadValue(EGX));
                        add_instruction(Opcode::Builder::smovr2(EGX, internalRegister->address));
                    }
                    break;
                }
                case class_object_data: {
                    generation_error("attempting to consume class object data into register!");
                    break;
                }
                case generic_object_data:
                case local_field_object_data:
                case new_class_data:
                case function_object_data:
                case field_object_data: {
                    generation_error("attempting to consume data object data into register!");
                    break;
                }
            }

            cc.machineData.type = no_data;
        } else {
            generation_error("attempting to consume empty machine data!");
        }
    } else {
        generation_error("attempt to consume data to unknown register!");
    }
}

void cast_machine_data(_register r, data_type type) {
    if(type == type_int8) { // todo: make sure that variables cannot be assigned after calling ++ before or after
        add_instruction(Opcode::Builder::mov8(r, r));
    } else if(type == type_int16) {
        add_instruction(Opcode::Builder::mov16(r, r));
    } else if(type == type_int32) {
        add_instruction(Opcode::Builder::mov32(r, r));
    } else if(type == type_int64) {
        add_instruction(Opcode::Builder::mov64(r, r));
    } else if(type == type_uint8) {
        add_instruction(Opcode::Builder::movu8(r, r));
    } else if(type == type_uint16) {
        add_instruction(Opcode::Builder::movu16(r, r));
    } else if(type == type_uint32) {
        add_instruction(Opcode::Builder::movu32(r, r));
    } else if(type == type_uint64) {
        add_instruction(Opcode::Builder::movu64(r, r));
    } else {
        generation_error("attempting to increment non-numeric type!");
    }
}

void decrement_machine_data(data_type type) {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case numeric_constant: {
                generation_error("attempting to dynamically increment constant! Fix this!");
                break;
            }
            case string_constant: {
                generation_error("attempting to consume string data into register!");
                break;
            }
            case numeric_local_field: {
                if(type == type_var) {
                    add_instruction(Opcode::Builder::isubl(1, cc.machineData.dataAddress));
                } else {
                    consume_machine_data(get_register(EGX));
                    set_machine_data(get_register(EGX));
                    add_instruction(Opcode::Builder::dec(EGX));
                    cast_machine_data(EGX, type);
                }
                break;
            }
            case numeric_register_data: {
                if(type == type_var) {
                    add_instruction(Opcode::Builder::dec((_register) cc.machineData.dataAddress));
                } else {
                    _register r = (_register) cc.machineData.dataAddress;
                    add_instruction(Opcode::Builder::dec(EGX));
                    cast_machine_data(r, type);
                }
                break;
            }
            case numeric_instance_field: {
                consume_machine_data(get_register(EGX));
                set_machine_data(get_register(EGX));
                add_instruction(Opcode::Builder::dec(EGX));

                if(type != type_var) {
                    cast_machine_data(EGX, type);
                }
                add_instruction(Opcode::Builder::imov(EGX));
                break;
            }
            case function_numeric_data: {
                consume_machine_data(get_register(EGX));
                add_instruction(Opcode::Builder::dec(EGX));
                set_machine_data(get_register(EGX));
                break;
            }
            case class_object_data: {
                generation_error("attempting to consume class object data into register!");
                break;
            }
            case generic_object_data:
            case local_field_object_data:
            case new_class_data:
            case function_object_data:
            case field_object_data: {
                generation_error("attempting to consume data object data into register!");
                break;
            }
        }
    } else {
        generation_error("attempting to consume empty machine data!");
    }
}

void increment_machine_data(data_type type) {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case numeric_constant: {
                generation_error("attempting to dynamically increment constant! Fix this!");
                break;
            }
            case string_constant: {
                generation_error("attempting to consume string data into register!");
                break;
            }
            case numeric_local_field: {
                if(type == type_var) {
                    add_instruction(Opcode::Builder::iaddl(1, cc.machineData.dataAddress));
                } else {
                    consume_machine_data(get_register(EGX));
                    set_machine_data(get_register(EGX));
                    add_instruction(Opcode::Builder::inc(EGX));
                    cast_machine_data(EGX, type);
                }
                break;
            }
            case numeric_register_data: {
                if(type == type_var) {
                    add_instruction(Opcode::Builder::inc((_register) cc.machineData.dataAddress));
                } else {
                    _register r = (_register) cc.machineData.dataAddress;
                    add_instruction(Opcode::Builder::inc(EGX));
                    cast_machine_data(r, type);
                }
                break;
            }
            case numeric_instance_field: {
                consume_machine_data(get_register(EGX));
                set_machine_data(get_register(EGX));
                add_instruction(Opcode::Builder::inc(EGX));

                if(type != type_var) {
                    cast_machine_data(EGX, type);
                }
                add_instruction(Opcode::Builder::imov(EGX));
                break;
            }
            case function_numeric_data: {
                consume_machine_data(get_register(EGX));
                add_instruction(Opcode::Builder::inc(EGX));
                set_machine_data(get_register(EGX));
                break;
            }
            case class_object_data: {
                generation_error("attempting to consume class object data into register!");
                break;
            }
            case generic_object_data:
            case local_field_object_data:
            case new_class_data:
            case function_object_data:
            case field_object_data: {
                generation_error("attempting to consume data object data into register!");
                break;
            }
        }
    } else {
        generation_error("attempting to consume empty machine data!");
    }
}

Int create_constant(double constant) {
    Int index;
    if((index = constantMap.indexof(constant)) == -1) {
        index = constantMap.size();
        constantMap.add(constant);
    }

    return index;
}

Int create_constant(string &constant) {
    Int index;
    if((index = stringMap.indexof(constant)) == -1) {
        index = constantMap.size();
        stringMap.add(constant);
    }

    return index;
}

