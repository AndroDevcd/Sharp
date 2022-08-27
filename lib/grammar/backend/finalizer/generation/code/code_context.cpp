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
    deleteList(dynamicInstructions);
    deleteList(labels);
    deleteList(tryCatchTable);
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

    for(Int i = 0; i < cc.tryCatchTable.size(); i++) {
        cc.ci->tryCatchTable.add(new try_catch_data(*cc.tryCatchTable.get(i)));
    }

    resolve_dynamic_instructions();
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

    cc.machineData.field = field;
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

            case null_data: {
                add_instruction(Opcode::Builder::pushNull());
                add_instruction(Opcode::Builder::popObject2());
                break;
            }

            case generic_object_data:
            case class_object_data:
            case field_object_data: {
                // do nothing
                cc.machineData.dataAddress = -1;
                break;
            }

            case array_object_data:
            case array_numeric_data:
            case new_class_data:
            case string_constant:
            case function_object_data: {
                add_instruction(Opcode::Builder::popObject2());
                break;
            }
        }

        cc.machineData.type = no_data;
    } else {
        generation_error("attempting to consume empty machine data!");
    }
}

void pop_machine_data_from_stack() {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case numeric_constant: {
                generation_error("attempting to assign numeric constant!");
                break;
            }
            case local_field_object_data: {
                add_instruction(Opcode::Builder::popl(cc.machineData.dataAddress));
                break;
            }

            case numeric_register_data: {
                add_instruction(Opcode::Builder::loadValue((_register) cc.machineData.dataAddress));
                break;
            }

            case numeric_instance_field: {
                add_instruction(Opcode::Builder::loadValue(EGX));
                if(cc.machineData.field->type.type < type_function_ptr) {
                    cast_machine_data(EGX, cc.machineData.field->type.type);
                }

                add_instruction(Opcode::Builder::imov(EGX));
                break;
            }

            case numeric_local_field: {
                if(cc.machineData.field->type.type < type_function_ptr) {
                    add_instruction(Opcode::Builder::loadValue(EGX));
                    cast_machine_data(EGX, cc.machineData.field->type.type);
                    add_instruction(Opcode::Builder::smovr2(EGX, cc.machineData.dataAddress));
                } else {
                    add_instruction(Opcode::Builder::ipopl(cc.machineData.dataAddress));
                }
                break;
            }

            case function_numeric_data: {
                generation_error("attempting to assign temporary numeric function data!");
                break;
            }

            case null_data: {
                generation_error("attempting to assign null data!");
                break;
            }

            case generic_object_data:
            case class_object_data:
            case field_object_data: {
                // do nothing
                add_instruction(Opcode::Builder::popObject());
                break;
            }

            case array_object_data:
            case array_numeric_data:
            case new_class_data:
            case string_constant:
            case function_object_data: {
                generation_error("attempting to assign stack data!");
                break;
            }
        }

        cc.machineData.type = no_data;
    } else {
        generation_error("attempting to consume empty machine data!");
    }
}


void clear_machine_data() {
    cc.machineData.type = no_data;
    cc.machineData.dataAddress = -1;
    cc.machineData.field = NULL;
}

void push_machine_data_to_stack() {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case numeric_constant: {
                if (floor(constantMap.get(cc.machineData.dataAddress)) == constantMap.get(cc.machineData.dataAddress)
                && constantMap.get(cc.machineData.dataAddress) < INT32_MAX) {
                    add_instruction(Opcode::Builder::istore(
                            (opcode_arg)constantMap.get(cc.machineData.dataAddress)));
                } else {
                    add_instruction(Opcode::Builder::ldc(
                            EGX, cc.machineData.dataAddress));
                    add_instruction(Opcode::Builder::rstore(EGX));
                }
                break;
            }
            case null_data: {
                add_instruction(Opcode::Builder::pushNull());
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
            case field_object_data: {
                add_instruction(Opcode::Builder::pushObject());
                break;
            }

            case local_field_object_data: {
                add_instruction(Opcode::Builder::pushl(cc.machineData.dataAddress));
                break;
            }

            case array_object_data:
            case array_numeric_data:
            case function_object_data:
            case function_numeric_data:
            case string_constant:
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

// this will pull data from register depending on data type
void consume_machine_data_from_register(internal_register *internalRegister) {
    if(internalRegister != NULL) {
        if (cc.machineData.type != no_data) {
            switch (cc.machineData.type) {
                case no_data:
                    break;
                case numeric_constant: {
                    generation_error("attempting to assign numeric constant!");
                    break;
                }
                case string_constant: {
                    generation_error("attempting to consume string data into register!");
                    break;
                }
                case numeric_local_field: {
                    if (internalRegister->type == normal_register) {
                        if(cc.machineData.field->type.type < type_function_ptr) {
                            cast_machine_data((_register) internalRegister->address, cc.machineData.field->type.type);
                        }

                        add_instruction(Opcode::Builder::smovr2(
                                (_register) internalRegister->address, cc.machineData.dataAddress));
                    } else {
                        if(cc.machineData.field->type.type < type_function_ptr) {
                            add_instruction(Opcode::Builder::loadl(
                                    EGX, internalRegister->address));

                            cast_machine_data(EGX, cc.machineData.field->type.type);
                            add_instruction(Opcode::Builder::smovr2(
                                    EGX, cc.machineData.dataAddress));
                        } else {
                            add_instruction(Opcode::Builder::smovr4(
                                    cc.machineData.dataAddress, internalRegister->address));
                        }
                    }
                    break;
                }
                case numeric_register_data: {
                    if (internalRegister->type == normal_register) {
                        if (internalRegister->address != cc.machineData.dataAddress) {
                            add_instruction(Opcode::Builder::movr(
                                    (_register) cc.machineData.dataAddress, (_register) internalRegister->address));
                        }
                    } else {
                        add_instruction(Opcode::Builder::loadl(
                                (_register) cc.machineData.dataAddress, internalRegister->address));
                    }
                    break;
                }
                case numeric_instance_field: {
                    if (internalRegister->type == normal_register) {
                        if(cc.machineData.field->type.type < type_function_ptr) {
                            cast_machine_data((_register) internalRegister->address, cc.machineData.field->type.type);
                        }

                        add_instruction(Opcode::Builder::imov((_register) internalRegister->address));
                    } else {
                        add_instruction(Opcode::Builder::loadl(EGX, internalRegister->address));
                        if(cc.machineData.field->type.type < type_function_ptr) {
                            cast_machine_data(EGX, cc.machineData.field->type.type);
                        }

                        add_instruction(Opcode::Builder::imov(EGX));
                    }
                    break;
                }
                case function_numeric_data: {
                    generation_error("attempting to set temporary data from register!");
                    break;
                }
                case class_object_data: {
                    generation_error("attempting to consume class object data into register!");
                    break;
                }
                case null_data:
                case generic_object_data:
                case local_field_object_data:
                case new_class_data:
                case array_object_data:
                case array_numeric_data:
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

// this will set this register supplied depending on current data type
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
                case null_data:
                case generic_object_data:
                case local_field_object_data:
                case new_class_data:
                case array_object_data:
                case array_numeric_data:
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
    if(type == type_int8) {
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
                    auto address = cc.machineData.dataAddress;
                    consume_machine_data(get_register(EGX));
                    set_machine_data(get_register(EGX));
                    add_instruction(Opcode::Builder::dec(EGX));
                    cast_machine_data(EGX, type);
                    add_instruction(Opcode::Builder::smovr2(EGX, address));
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
            case null_data:
            case generic_object_data:
            case local_field_object_data:
            case new_class_data:
            case array_object_data:
            case array_numeric_data:
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
                    auto address = cc.machineData.dataAddress;
                    consume_machine_data(get_register(EGX));
                    set_machine_data(get_register(EGX));
                    add_instruction(Opcode::Builder::inc(EGX));
                    cast_machine_data(EGX, type);
                    add_instruction(Opcode::Builder::smovr2(EGX, address));
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
            case null_data:
            case generic_object_data:
            case local_field_object_data:
            case new_class_data:
            case array_object_data:
            case array_numeric_data:
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

Int create_label() {
    auto id = UUIDGenerator++;
    create_label(id);
    return id;
}

void create_label(Int id) {
    if(find_label(id) == NULL) {
        cc.labels.add(new internal_label(id));
    } else {
        generation_error("attempting to create label with same id!");
    }
}

void create_finally_data(Int id, Int parentId) {
    if(id != -1) {
        try_catch_data *tcd =
                find_try_catch_data(parentId);

        if(tcd != NULL) {
            finally_data *fd =
                    tcd->finallyData;

            if(fd == NULL) {
                fd = new finally_data(id);
                tcd->finallyData = fd;
            } else {
                generation_error("attempting to create already created finally data!");
            }
        } else {
            generation_error("attempting to create finally data from unknown parent!");
        }
    } else {
        generation_error("attempting to create finally data without id!");
    }
}

finally_data* get_finally_data(Int id, try_catch_data *tcd) {
    if(tcd != NULL) {
        finally_data *fd =
                tcd->finallyData;

        if(fd != NULL) {
            if (fd->id == id) {
                return fd;
            } else {
                generation_error("attempt to get finally data with incorrect id;");
                return NULL;
            }
        } else {
            return NULL;
        }
    } else {
        generation_error("attempt to get finally data from unknown parent!");
        return NULL;
    }
}


void create_catch_data(Int id, Int parentId) {
    if(id != -1) {
        try_catch_data *tcd =
                find_try_catch_data(parentId);

        if(tcd != NULL) {
            catch_data *cd =
                    find_catch_data(id, tcd);

            if(cd == NULL) {
                cd = new catch_data(id);
                tcd->catchTable.add(cd);
            } else {
                generation_error("attempting to create already created catch data!");
            }
        } else {
            generation_error("attempting to create catch data from unknown parent!");
        }
    } else {
        generation_error("attempting to create catch data without id!");
    }
}

catch_data* find_catch_data(Int id, try_catch_data *tcd) {
    if(tcd != NULL) {
        for (Int i = 0; i < tcd->catchTable.size(); i++) {
            if (tcd->catchTable.get(i)->id == id)
                return tcd->catchTable.get(i);
        }
    }
    
    return NULL;
}

try_catch_data* find_try_catch_data(Int id) {
    for(Int i = 0; i < cc.tryCatchTable.size(); i++) {
        if(cc.tryCatchTable.get(i)->id == id)
            return cc.tryCatchTable.get(i);
    }

    return NULL;
}

void create_try_catch_data(Int id) {
    if(id != -1) {
        try_catch_data *tcd =
                find_try_catch_data(id);

        if(tcd == NULL) {
            tcd = new try_catch_data(id);
            cc.tryCatchTable.add(tcd);
        } else {
            generation_error("attempting to create already created try catch data!");
        }
    } else {
        generation_error("attempting to create try catch data without id!");
    }
}

void set_label(internal_label *l) {
    if(l != NULL) {
        if (l->address == -1) {
            l->address = cc.instructions.size();
        } else {
            generation_error("attempting to set label that was previously set!");
        }
    } else {
        generation_error("attempting to set unknown label!");
    }
}

internal_label* find_label(Int id) {
    for(Int i = 0; i < cc.labels.size(); i++) {
        if(cc.labels.get(i)->id == id)
            return cc.labels.get(i);
    }

    return NULL;
}

opcode_arg get_dynamic_arg_value(dynamic_argument &da) {
    switch(da.type) {
        case regular_argument: {
            return da.argData;
        }
        case label_argument: {
            internal_label *label = find_label(da.argData);

            if(label != NULL && label->address != -1) {
                return label->address;
            } else {
                generation_error("attempting to get data from uninitialized label!");
            }
        }
        default: {
            generation_error("attempting to get data from unknown dynamic argument type!");
            break;
        }
    }

    return -1;
}

void create_dynamic_instruction(dynamic_instruction di) {
    if(dynamic_instruction_supported(di.opcode)) {
        di.ip = cc.instructions.size();
        for(int i = 0; i < di.size; i++) {
            cc.instructions.add(0);
        }

        cc.dynamicInstructions.add(new dynamic_instruction(di));
    } else {
        generation_error("attempting to create unsupported dynamic instruction!");
    }
}

void resolve_dynamic_instruction(List<opcode_instr> &instructions, dynamic_instruction &di) {
    switch(di.opcode) {
        case Opcode::MOVI: {
            opcode_instr *instr = Opcode::Builder::movi(
                    get_dynamic_arg_value(di.arg1), (_register) get_dynamic_arg_value(di.arg2));

            Int instrPos = 0;
            for(Int i = 0; i < INSTRUCTION_BUFFER_SIZE; i++) {
                instructions.get(di.ip + i) = instr[instrPos++];
            }
            break;
        }
        case Opcode::JE: {
            instructions.get(di.ip) = Opcode::Builder::je(get_dynamic_arg_value(di.arg1));
            break;
        }
        case Opcode::JNE: {
            instructions.get(di.ip) = Opcode::Builder::jne(get_dynamic_arg_value(di.arg1));
            break;
        }
        case Opcode::JMP: {
            instructions.get(di.ip) = Opcode::Builder::jmp(get_dynamic_arg_value(di.arg1));
            break;
        }
        default:
            generation_error("attempting to inject unsupported dynamic instruction!");
            break;
    }
}

void resolve_dynamic_instructions() {
    for(Int i = 0; i < cc.dynamicInstructions.size(); i++) {
        resolve_dynamic_instruction(cc.instructions, *cc.dynamicInstructions.get(i));
    }
}

bool dynamic_instruction_supported(int opcode) {
    switch(opcode) {
        case Opcode::MOVI:
        case Opcode::JE:
        case Opcode::JNE:
        case Opcode::JMP:
            return true;

        default:
            return false;
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
        index = stringMap.size();
        stringMap.add(constant);
    }

    return index;
}

