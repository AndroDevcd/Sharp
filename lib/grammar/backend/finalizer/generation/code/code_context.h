//
// Created by bknun on 8/9/2022.
//

#ifndef SHARP_CODE_CONTEXT_H
#define SHARP_CODE_CONTEXT_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../../../runtime/Opcode.h"
#include "register_allocator.h"

struct code_info;
struct line_info;
struct internal_register;

enum machine_data_type {
    no_data,
    numeric_constant,
    string_constant,
    numeric_instance_field,
    numeric_local_field,
    class_object_data,
    field_object_data,
    local_field_object_data,
    numeric_register_data
};

struct machine_data {
    machine_data()
    :
        dataAddress(0),
        type(no_data)
    {}

    Int dataAddress;
    machine_data_type type;
};

struct code_context {
    code_context()
            :
            ci(NULL),
            container(NULL)
    {}

    code_context(const code_context &cc)
            :
            ci(cc.ci),
            instructions(cc.instructions),
            container(cc.container)
    {
    }

    ~code_context() {
        free();
    }

    void free();

    code_info* ci;
    List<internal_register*> registers;
    List<line_info*> lineTable;
    List<opcode_instr> instructions;
    sharp_function *container;
    machine_data machineData;
};

void set_machine_data(internal_register *internalRegister, bool force = false);
void set_machine_data(sharp_field *field, bool localField, bool force = false);
void set_machine_data(machine_data_type dataType, Int dataAddr, bool force = false);
void set_machine_data(machine_data_type dataType);
void consume_machine_data(); // consumes to ebx
void consume_machine_data(internal_register *);
void increment_machine_data(data_type);
void decrement_machine_data(data_type);

Int create_constant(double constant);
Int create_constant(string &constant);

void update_context(code_info* ci, sharp_function *container);
void flush_context();

extern code_context cc;

#endif //SHARP_CODE_CONTEXT_H
