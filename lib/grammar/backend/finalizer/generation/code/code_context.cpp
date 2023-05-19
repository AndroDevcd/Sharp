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

void set_fp_offset() {
    bool staticFunc = check_flag(cc.container->flags, flag_static);
    Int paramSize = cc.container->parameters.size();
    if(cc.container->returnType == type_nil) {
        cc.ci->fpOffset = paramSize + (!staticFunc ? 0 : -1);
    } else {
        if(staticFunc) {
            if(paramSize == 0) {
                cc.ci->fpOffset =  -1;
            } else {
                cc.ci->fpOffset =  cc.container->parameters.size() - 1;
            }
        } else {
            if(paramSize == 0) {
                cc.ci->fpOffset =  0;
            } else {
                cc.ci->fpOffset = cc.container->parameters.size();
            }
        }
    }
}

void set_sp_offset() {
    bool staticFunc = check_flag(cc.container->flags, flag_static);
    Int paramSize = cc.container->parameters.size();
    if(cc.container->returnType == type_nil) {
        cc.ci->spOffset = paramSize + (!staticFunc ? 1 : 0);
    } else {
        if(staticFunc) {
            if(paramSize == 0) {
                cc.ci->spOffset = -1;
            } else {
                cc.ci->spOffset = cc.container->parameters.size() - 1;
            }
        } else {
            if(paramSize == 0) {
                cc.ci->spOffset = 0;
            } else {
                cc.ci->spOffset = cc.container->parameters.size();
            }
        }
    }
}

void set_frame_stack_offset() {
    bool staticFunc = check_flag(cc.container->flags, flag_static);
    Int paramSize = cc.container->parameters.size();
    Int exclusiveStackSize = cc.ci->stackSize;
    if(!staticFunc) exclusiveStackSize--;
    Int additionalStackPaceRequires = exclusiveStackSize - paramSize;
    cc.ci->frameStackOffset = additionalStackPaceRequires > 0 ? additionalStackPaceRequires : 0;
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
    set_fp_offset();
    set_sp_offset();
    set_frame_stack_offset();
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
    di.ip = cc.instructions.size();
    for(int i = 0; i < di.size; i++) {
        cc.instructions.add(0);
    }

    cc.dynamicInstructions.add(new dynamic_instruction(di));
}

void update_dynamic_instruction(List<opcode_instr> &instructions, dynamic_instruction &di, opcode_instr *instr) {
    Int instrPos = 0;
    for(Int i = 0; i < INSTRUCTION_BUFFER_SIZE; i++) {
        instructions.get(di.ip + i) = instr[instrPos++];
    }
}

void update_dynamic_instruction(List<opcode_instr> &instructions, dynamic_instruction &di, opcode_instr instr) {
    instructions.get(di.ip) = instr;
}

void resolve_dynamic_instruction(List<opcode_instr> &instructions, dynamic_instruction &di) {
    switch(di.opcode) {
        case Opcode::NOP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::nop());
            break;
        }
        case Opcode::INT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_int((interrupt_flag)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOVI: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movi(get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::RET: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ret(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::HLT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::hlt());
            break;
        }
        case Opcode::NEWARRAY: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::newVarArray((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::CAST: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::cast(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOV8: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mov8((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOV16: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mov16((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOV32: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mov32((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOV64: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mov64((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOVU8: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movu8((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOVU16: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movu16((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOVU32: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movu32((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOVU64: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movu64((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::RSTORE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::rstore((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ADD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::add(
                        (_register)get_dynamic_arg_value(di.arg1),
                        (_register)get_dynamic_arg_value(di.arg2),
                        (_register)get_dynamic_arg_value(di.arg3)
                    )
            );
            break;
        }
        case Opcode::SUB: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::sub(
                            (_register)get_dynamic_arg_value(di.arg1),
                            (_register)get_dynamic_arg_value(di.arg2),
                            (_register)get_dynamic_arg_value(di.arg3)
                    )
            );
            break;
        }
        case Opcode::MUL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mul(
                            (_register)get_dynamic_arg_value(di.arg1),
                            (_register)get_dynamic_arg_value(di.arg2),
                            (_register)get_dynamic_arg_value(di.arg3)
                    )
            );
            break;
        }
        case Opcode::DIV: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::div(
                           (_register)get_dynamic_arg_value(di.arg1),
                           (_register)get_dynamic_arg_value(di.arg2),
                           (_register)get_dynamic_arg_value(di.arg3)
                   )
            );
            break;
        }
        case Opcode::MOD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mod(
                           (_register)get_dynamic_arg_value(di.arg1),
                           (_register)get_dynamic_arg_value(di.arg2),
                           (_register)get_dynamic_arg_value(di.arg3)
                   )
            );
            break;
        }
        case Opcode::IADD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::iadd(
                           (_register)get_dynamic_arg_value(di.arg1),
                           get_dynamic_arg_value(di.arg2)
                   )
            );
            break;
        }
        case Opcode::ISUB: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::isub(
                            (_register)get_dynamic_arg_value(di.arg1),
                            get_dynamic_arg_value(di.arg2)
                    )
            );
            break;
        }
        case Opcode::IMUL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::imul(
                            (_register)get_dynamic_arg_value(di.arg1),
                            get_dynamic_arg_value(di.arg2)
                    )
            );
            break;
        }
        case Opcode::IDIV: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::idiv(
                            (_register)get_dynamic_arg_value(di.arg1),
                            get_dynamic_arg_value(di.arg2)
                    )
            );
            break;
        }
        case Opcode::IMOD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::imod(
                            (_register)get_dynamic_arg_value(di.arg1),
                            get_dynamic_arg_value(di.arg2)
                    )
            );
            break;
        }
        case Opcode::POP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::pop());
            break;
        }
        case Opcode::INC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::inc((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::DEC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dec((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOVR: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movr((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IALOAD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::iaload((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::BRH: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::brh());
            break;
        }
        case Opcode::IFE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ife());
            break;
        }
        case Opcode::IFNE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ifne());
            break;
        }
        case Opcode::LT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::lt((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::GT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::gt((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::LTE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::le((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::GTE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ge((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MOVL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOVSL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movsl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::SIZEOF: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_sizeof((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::PUT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::put((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::PUTC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::putc((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::CHECKLEN: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::checklen((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::LOADPC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::loadpc((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::PUSHOBJ: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::pushObject());
            break;
        }
        case Opcode::DEL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::del());
            break;
        }
        case Opcode::CALL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::call(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::NEWCLASS: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::newClass(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOVN: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movn(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::SLEEP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::sleep((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::TEST: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::te((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::TNE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::tne((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::LOCK: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::lock());
            break;
        }
        case Opcode::ULOCK: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::unlock());
            break;
        }
        case Opcode::MOVG: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movg(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOVND: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movnd((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::NEWOBJARRAY: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::newObjectArray((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::NOT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_not((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::SKIP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::skip(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::LOADVAL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::loadValue((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::SHL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::shl(
                    (_register)get_dynamic_arg_value(di.arg1),
                    (_register)get_dynamic_arg_value(di.arg2),
                    (_register)get_dynamic_arg_value(di.arg3)
                  )
            );
            break;
        }
        case Opcode::SHR: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::shr(
                    (_register)get_dynamic_arg_value(di.arg1),
                    (_register)get_dynamic_arg_value(di.arg2),
                    (_register)get_dynamic_arg_value(di.arg3)
                 )
            );
            break;
        }
        case Opcode::SKPE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::skipife((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::SKNE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::skipifne((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::AND: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_and((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::UAND: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::uand((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::OR: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_or((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::XOR: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_xor((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::THROW: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::_throw());
            break;
        }
        case Opcode::CHECKNULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::checkNull((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::RETURNOBJ: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::returnObject());
            break;
        }
        case Opcode::NEWCLASSARRAY: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::newClassArray((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::NEWSTRING: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::newString(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ADDL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::addl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::SUBL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::subl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mull((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DIVL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::divl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MODL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::modl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IADDL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::iaddl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ISUBL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::isubl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IMULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::imull(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IDIVL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::idivl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IMODL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::imodl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::LOADL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::loadl((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::POPOBJ: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::popObject());
            break;
        }
        case Opcode::SMOVR: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::smovr((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ANDL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::andl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ORL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::orl((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::XORL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::xorl((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::RMOV: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::rmov((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::SMOV: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::smov((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::RETURNVAL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::returnValue((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ISTORE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::istore(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::SMOVR_2: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::smovr2((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::SMOVR_3: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::smovr3(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ISTOREL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::istorel(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::POPL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::popl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::PUSHNULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::pushNull());
            break;
        }
        case Opcode::IPUSHL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ipushl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::PUSHL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::pushl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ITEST: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::itest((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::INVOKE_DELEGATE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::invokeDelegate(
                    get_dynamic_arg_value(di.arg1),
                    get_dynamic_arg_value(di.arg2),
                    get_dynamic_arg_value(di.arg3)
                 )
            );
            break;
        }
        case Opcode::GET: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::get((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ISADD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::isadd(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IPOPL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ipopl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::CMP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::cmp((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::CALLD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::calld((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::VARCAST: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::varCast(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::TLS_MOVL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::tlsMovl(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::DUP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dup());
            break;
        }
        case Opcode::POPOBJ_2: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::popObject2());
            break;
        }
        case Opcode::SWAP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::swap());
            break;
        }
        case Opcode::LDC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ldc((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::NEG: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::neg((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::JNE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::jne(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::JE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::je(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::JMP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::jmp(get_dynamic_arg_value(di.arg1)));
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

