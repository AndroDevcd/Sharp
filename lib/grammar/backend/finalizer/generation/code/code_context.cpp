//
// Created by bknun on 8/9/2022.
//

#include "code_context.h"
#include "code_info.h"
#include "../field_generator.h"
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
    cc.lastInstruction = -1;
    cc.previousInstruction = -1;
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

    // Tag for the bytecode optimizer: remember which non-branch instructions
    // were given a label (a code address) as an operand. Nothing about the
    // generated code changes, see code_info::labelOperands.
    for(Int i = 0; i < cc.dynamicInstructions.size(); i++) {
        dynamic_instruction *di = cc.dynamicInstructions.get(i);
        bool usesLabel = di->arg1.type == label_argument || di->arg2.type == label_argument
                || di->arg3.type == label_argument;

        if(usesLabel && di->opcode != Opcode::JMP && di->opcode != Opcode::JE && di->opcode != Opcode::JNE)
            cc.ci->labelOperands.add(di->ip);
    }

    cc.ci->stackSize = (check_flag(cc.container->flags, flag_static) ? 0 : 1) +
            cc.container->locals.size() + (cc.registers.size() - MIN_REGISTERS);
    set_fp_offset();
    set_sp_offset();
    set_frame_stack_offset();
    cc.ci->code.appendAll(cc.instructions);
    cc.free();
}

/**
 * ELEMENTS OF A NUMBER ARRAY
 *
 * A number array stores its elements at their declared size (1, 2, 4 or 8
 * bytes; element_size() in lib/runtime/memory/sharp_object.h), so reading and
 * writing one needs the instruction for that size. The array is in the object
 * pointer and the index in a register, as before.
 *
 *      bytes : _int8[]     aload8   / astore8      (sign extended to 64 bits)
 *      flags : _uint8[]    aloadu8  / astore8      (zero extended)
 *      ids   : _int32[]    aload32  / astore32
 *      big   : _int64[]    aload64  / astore64
 *      vals  : decimal[]   aload64  / astore64     (8 bytes copied as they are)
 *
 * A store cuts the value to the element size by itself; unsigned and signed
 * elements of one size are stored the same way.
 */
opcode_instr array_load(data_type elementType, _register out, _register index) {
    switch(elementType) {
        case type_int8:   return Opcode::Builder::aload8(out, index);
        case type_uint8:  return Opcode::Builder::aloadu8(out, index);
        case type_int16:  return Opcode::Builder::aload16(out, index);
        case type_uint16: return Opcode::Builder::aloadu16(out, index);
        case type_int32:  return Opcode::Builder::aload32(out, index);
        case type_uint32: return Opcode::Builder::aloadu32(out, index);
        default:          return Opcode::Builder::aload64(out, index);  // _int64, _uint64, function pointers, decimal
    }
}

opcode_instr array_store(data_type elementType, _register index, _register in) {
    switch(elementType) {
        case type_int8: case type_uint8:   return Opcode::Builder::astore8(index, in);
        case type_int16: case type_uint16: return Opcode::Builder::astore16(index, in);
        case type_int32: case type_uint32: return Opcode::Builder::astore32(index, in);
        default:                           return Opcode::Builder::astore64(index, in);
    }
}

bool is_array_load(opcode_instr instruction, _register out, _register index) {
    static const data_type sizes[] = {
        type_int8, type_uint8, type_int16, type_uint16, type_int32, type_uint32, type_int64
    };

    for(data_type size : sizes) {
        if(instruction == array_load(size, out, index))
            return true;
    }
    return false;
}

/**
 * Go from the object in the object pointer to its field `field`.
 *
 *   - a reference field (an object, an array): the pointer moves onto the
 *     field's slot,  movn <slot>
 *   - a number field: nothing is emitted. The number is in the slot of the
 *     object the pointer is already on, and GETF / PUTF take the slot as an
 *     operand (numeric_instance_field in consume_machine_data and friends).
 *
 * Call set_machine_data(field, false) afterwards as before.
 */
void move_to_field(sharp_field *field) {
    if(is_numeric_type(field->type) && !field->type.isArray)
        return;

    add_instruction(Opcode::Builder::movn(get_or_initialize_code(field)->address));
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

    // A register (or the frame slot a register was spilled to) is not a declared
    // field. The field of whatever value was current before must not stay
    // attached: storing into a spilled register looked at it and narrowed the
    // value to THAT field's type - in `index.value % bits_per_word` the decimal
    // on the right was cut to the _int32 of the field on the left.
    cc.machineData.field = NULL;
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
            case integer_constant:
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
            case numeric_tls_field:
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
            case integer_constant:
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

                add_instruction(Opcode::Builder::putf(EGX, cc.machineData.dataAddress));
                break;
            }

            case numeric_local_field: {
                // field == NULL: not a declared local but a compiler temporary (an expression deep
                // enough to run out of registers spills into stack slots, see allocate_register): no cast
                if(cc.machineData.field != NULL && cc.machineData.field->type.type < type_function_ptr) {
                    add_instruction(Opcode::Builder::loadValue(EGX));
                    cast_machine_data(EGX, cc.machineData.field->type.type);
                    add_instruction(Opcode::Builder::smovr2(EGX, cc.machineData.dataAddress));
                } else {
                    add_instruction(Opcode::Builder::ipopl(cc.machineData.dataAddress));
                }
                break;
            }

            case numeric_tls_field: {
                add_instruction(Opcode::Builder::loadValue(EBX));
                add_instruction(Opcode::Builder::movabs(cc.machineData.dataAddress));
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

/**
 * INTEGER CONSTANTS
 *
 * An integer constant is not in the constant table (that table holds decimals):
 * its value travels in the machine data (integer_constant) until it is needed.
 *
 *      movi <value>, reg       when the value fits in the instruction's second
 *                              word (a signed 32-bit number)
 *      ldc reg, <index>        for larger values: for now they still come from
 *      d2i reg, reg            the decimal table, which is exact up to 2^53.
 *                              (An integer constant table replaces this when
 *                              integers get their own 64-bit storage.)
 */
static bool fits_in_instruction(Int value) {
    return value > INT32_MIN && value < INT32_MAX;
}

static void load_integer_constant(_register reg, Int value) {
    if(fits_in_instruction(value)) {
        add_instruction(Opcode::Builder::movi((opcode_arg)value, reg));
    } else {
        add_instruction(Opcode::Builder::ldc(reg, create_constant((double)value)));
        add_instruction(Opcode::Builder::d2i(reg, reg));
    }
}

void push_machine_data_to_stack() {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case integer_constant: {
                // push an integer: `istore` holds it in the instruction when it
                // fits in 32 bits, a larger one goes through a register
                if(fits_in_instruction(cc.machineData.dataAddress)) {
                    add_instruction(Opcode::Builder::istore((opcode_arg)cc.machineData.dataAddress));
                } else {
                    load_integer_constant(EGX, cc.machineData.dataAddress);
                    add_instruction(Opcode::Builder::rstore(EGX));
                }
                break;
            }
            case numeric_constant: {
                // push a decimal: `distore` when it is a whole number that fits
                // in the instruction (2.0), the constant table otherwise (0.5)
                double value = constantMap.get(cc.machineData.dataAddress);
                // (no lower bound was checked once: -5000000000 went down the
                //  32 bit path and was pushed as -2147483648)
                if (floor(value) == value && value < INT32_MAX && value > INT32_MIN) {
                    add_instruction(Opcode::Builder::distore((opcode_arg)value));
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
            case numeric_tls_field: {
                add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));
                add_instruction(Opcode::Builder::rstore(EBX));
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
                case integer_constant:
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
                        if(cc.machineData.field != NULL && cc.machineData.field->type.type < type_function_ptr) {
                            cast_machine_data((_register) internalRegister->address, cc.machineData.field->type.type);
                        }

                        add_instruction(Opcode::Builder::smovr2(
                                (_register) internalRegister->address, cc.machineData.dataAddress));
                    } else {
                        if(cc.machineData.field != NULL && cc.machineData.field->type.type < type_function_ptr) {
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
                case numeric_tls_field: {
                    if (internalRegister->type == normal_register) {
                        if(internalRegister->address != EBX)
                            add_instruction(Opcode::Builder::movr(EBX, (_register)internalRegister->address));
                    } else {
                        add_instruction(Opcode::Builder::loadl(
                                EBX, internalRegister->address));
                    }

                    add_instruction(Opcode::Builder::movabs(cc.machineData.dataAddress));
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

                        add_instruction(Opcode::Builder::putf((_register) internalRegister->address, cc.machineData.dataAddress));
                    } else {
                        add_instruction(Opcode::Builder::loadl(EGX, internalRegister->address));
                        if(cc.machineData.field->type.type < type_function_ptr) {
                            cast_machine_data(EGX, cc.machineData.field->type.type);
                        }

                        add_instruction(Opcode::Builder::putf(EGX, cc.machineData.dataAddress));
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
                case integer_constant: {
                    if (internalRegister->type == normal_register) {
                        load_integer_constant((_register) internalRegister->address, cc.machineData.dataAddress);
                    } else {
                        load_integer_constant(EGX, cc.machineData.dataAddress);
                        add_instruction(Opcode::Builder::smovr2(EGX, internalRegister->address));
                    }
                    break;
                }
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
                case numeric_tls_field: {
                    if (internalRegister->type == normal_register) {
                        add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));

                        if(internalRegister->address == EBX) {
                            add_instruction(Opcode::Builder::movr((_register) internalRegister->address, EBX));
                        }
                    } else {
                        add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));
                        add_instruction(Opcode::Builder::smovr2(
                                EBX, internalRegister->address));
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
                    // a number field of the object in the object pointer:
                    // dataAddress is the field's slot (see move_to_field)
                    if (internalRegister->type == normal_register) {
                        add_instruction(Opcode::Builder::getf((_register) internalRegister->address, cc.machineData.dataAddress));
                    } else {
                        add_instruction(Opcode::Builder::getf(EGX, cc.machineData.dataAddress));
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

/**
 * ++ and -- on a number in a register: DINC / DDEC for a decimal (`var`),
 * INC / DEC for every integer type.
 */
static opcode_instr inc_register(data_type type, _register r) {
    return type == type_var ? Opcode::Builder::dinc(r) : Opcode::Builder::inc(r);
}

static opcode_instr dec_register(data_type type, _register r) {
    return type == type_var ? Opcode::Builder::ddec(r) : Opcode::Builder::dec(r);
}

void decrement_machine_data(data_type type) {
    if(cc.machineData.type != no_data) {
        switch(cc.machineData.type) {
            case no_data: break;
            case integer_constant:
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
                    add_instruction(Opcode::Builder::disubl(1, cc.machineData.dataAddress));
                } else {
                    auto address = cc.machineData.dataAddress;
                    consume_machine_data(get_register(EGX));
                    set_machine_data(get_register(EGX));
                    add_instruction(dec_register(type, EGX));
                    cast_machine_data(EGX, type);
                    add_instruction(Opcode::Builder::smovr2(EGX, address));
                }
                break;
            }
            case numeric_tls_field: {
                if(type == type_var) {
                    add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));
                    add_instruction(dec_register(type, EBX));
                    add_instruction(Opcode::Builder::movabs(cc.machineData.dataAddress));
                } else {
                    add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));
                    add_instruction(dec_register(type, EBX));
                    cast_machine_data(EBX, type);
                    add_instruction(Opcode::Builder::movabs(cc.machineData.dataAddress));
                }
                break;
            }
            case numeric_register_data: {
                if(type == type_var) {
                    add_instruction(dec_register(type, (_register) cc.machineData.dataAddress));
                } else {
                    _register r = (_register) cc.machineData.dataAddress;
                    add_instruction(dec_register(type, r));
                    cast_machine_data(r, type);
                }
                break;
            }
            case numeric_instance_field: {
                Int fieldSlot = cc.machineData.dataAddress;
                consume_machine_data(get_register(EGX));
                set_machine_data(get_register(EGX));
                add_instruction(dec_register(type, EGX));

                if(type != type_var) {
                    cast_machine_data(EGX, type);
                }
                add_instruction(Opcode::Builder::putf(EGX, fieldSlot));
                break;
            }
            case function_numeric_data: {
                consume_machine_data(get_register(EGX));
                add_instruction(dec_register(type, EGX));
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
            case integer_constant:
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
                    add_instruction(Opcode::Builder::diaddl(1, cc.machineData.dataAddress));
                } else {
                    auto address = cc.machineData.dataAddress;
                    consume_machine_data(get_register(EGX));
                    set_machine_data(get_register(EGX));
                    add_instruction(inc_register(type, EGX));
                    cast_machine_data(EGX, type);
                    add_instruction(Opcode::Builder::smovr2(EGX, address));
                }
                break;
            }
            case numeric_tls_field: {
                if(type == type_var) {
                    add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));
                    add_instruction(inc_register(type, EBX));
                    add_instruction(Opcode::Builder::movabs(cc.machineData.dataAddress));
                } else {
                    add_instruction(Opcode::Builder::loadabs(cc.machineData.dataAddress));
                    add_instruction(inc_register(type, EBX));
                    cast_machine_data(EBX, type);
                    add_instruction(Opcode::Builder::movabs(cc.machineData.dataAddress));
                }
                break;
            }
            case numeric_register_data: {
                if(type == type_var) {
                    add_instruction(inc_register(type, (_register) cc.machineData.dataAddress));
                } else {
                    _register r = (_register) cc.machineData.dataAddress;
                    add_instruction(inc_register(type, r));
                    cast_machine_data(r, type);
                }
                break;
            }
            case numeric_instance_field: {
                Int fieldSlot = cc.machineData.dataAddress;
                consume_machine_data(get_register(EGX));
                set_machine_data(get_register(EGX));
                add_instruction(inc_register(type, EGX));

                if(type != type_var) {
                    cast_machine_data(EGX, type);
                }
                add_instruction(Opcode::Builder::putf(EGX, fieldSlot));
                break;
            }
            case function_numeric_data: {
                consume_machine_data(get_register(EGX));
                add_instruction(inc_register(type, EGX));
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
            cc.lastInstruction = -1;    // code jumps here: what ran just before is not known
            cc.previousInstruction = -1;
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
    cc.lastInstruction = -1;            // a placeholder, filled in later
    cc.previousInstruction = -1;

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
        case Opcode::DMOVI: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dmovi(get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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
        case Opcode::D2I: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::d2i((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::I2D: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::i2d((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DADD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dadd(
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
        case Opcode::DSUB: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dsub(
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
        case Opcode::DMUL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dmul(
                            (_register)get_dynamic_arg_value(di.arg1),
                            (_register)get_dynamic_arg_value(di.arg2),
                            (_register)get_dynamic_arg_value(di.arg3)
                    )
            );
            break;
        }
        case Opcode::DDIV: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ddiv(
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
        case Opcode::DIADD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::diadd(
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
        case Opcode::DISUB: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::disub(
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
        case Opcode::DIMUL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dimul(
                            (_register)get_dynamic_arg_value(di.arg1),
                            get_dynamic_arg_value(di.arg2)
                    )
            );
            break;
        }
        case Opcode::DIDIV: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::didiv(
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
        case Opcode::DINC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dinc((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::DEC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dec((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::DDEC: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ddec((_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::MOVR: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::movr((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOAD64: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aload64((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOADU32: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aloadu32((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOAD32: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aload32((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOADU16: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aloadu16((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOAD16: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aload16((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOADU8: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aloadu8((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ALOAD8: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::aload8((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DLT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dlt((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::GT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::gt((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DGT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dgt((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::LTE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::le((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DLTE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dlte((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::GTE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ge((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DGTE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dgte((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DPUT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dput((_register)get_dynamic_arg_value(di.arg1)));
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
        case Opcode::DTEST: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dtest((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::TNE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::tne((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DTNE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dtne((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DNOT: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dnot((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DADDL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::daddl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::SUBL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::subl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DSUBL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dsubl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::MULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::mull((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DMULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dmull((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DDIVL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::ddivl((_register)get_dynamic_arg_value(di.arg1),get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DIADDL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::diaddl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::ISUBL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::isubl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DISUBL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::disubl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::IMULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::imull(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DIMULL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dimull(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::DIDIVL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::didivl(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
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
        case Opcode::GETF: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::getf((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
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
        case Opcode::ASTORE64: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::astore64((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ASTORE32: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::astore32((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ASTORE16: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::astore16((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::ASTORE8: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::astore8((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg1)));
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
        case Opcode::DISTORE: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::distore(get_dynamic_arg_value(di.arg1)));
            break;
        }
        case Opcode::SMOVR_2: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::smovr2((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
            break;
        }
        case Opcode::PUTF: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::putf((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DISTOREL: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::distorel(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DISADD: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::disadd(get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DCMP: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dcmp((_register)get_dynamic_arg_value(di.arg1), get_dynamic_arg_value(di.arg2)));
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
        case Opcode::DNEG: {
            update_dynamic_instruction(instructions, di, Opcode::Builder::dneg((_register)get_dynamic_arg_value(di.arg1), (_register)get_dynamic_arg_value(di.arg2)));
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

