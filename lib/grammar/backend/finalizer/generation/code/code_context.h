//
// Created by bknun on 8/9/2022.
//

#ifndef SHARP_CODE_CONTEXT_H
#define SHARP_CODE_CONTEXT_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../../../core/opcode/opcode.h"
#include "register_allocator.h"

struct code_info;
struct try_catch_data;
struct catch_data;
struct finally_data;
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
    numeric_register_data,
    generic_object_data,
    new_class_data,
    function_object_data,
    function_numeric_data,
    null_data,
    array_object_data,
    array_numeric_data,
};

struct machine_data {
    machine_data()
    :
        dataAddress(0),
        type(no_data),
        field(NULL)
    {}

    machine_data(const machine_data &md)
            :
            dataAddress(md.dataAddress),
            type(md.type),
            field(md.field)
    {}

    sharp_field *field;
    Int dataAddress;
    machine_data_type type;
};

enum opcode_arg_type {
    no_argument,
    regular_argument,
    label_argument
};

struct dynamic_argument {
    dynamic_argument()
            :
            type(no_argument),
            argData(0)
    {}

    dynamic_argument(opcode_arg_type type, Int data)
            :
            type(type),
            argData(data)
    {}

    dynamic_argument(const dynamic_argument &da)
            :
            type(da.type),
            argData(da.argData)
    {}

    opcode_arg_type type;
    Int argData;
};

struct dynamic_instruction {
    dynamic_instruction()
            :
            opcode(0),
            ip(0),
            size(0),
            arg1(),
            arg2(),
            arg3()
    {}

    dynamic_instruction(
            int op,
            int size,
            dynamic_argument a1,
            dynamic_argument a2,
            dynamic_argument a3
    )
            :
            opcode(op),
            ip(0),
            size(size),
            arg1(a1),
            arg2(a2),
            arg3(a3)
    {}

    dynamic_instruction(
            int op,
            int size,
            dynamic_argument a1
    )
            :
            opcode(op),
            ip(0),
            size(size),
            arg1(a1),
            arg2(),
            arg3()
    {}

    dynamic_instruction(
            int op,
            int size,
            dynamic_argument a1,
            dynamic_argument a2
    )
            :
            opcode(op),
            ip(0),
            size(size),
            arg1(a1),
            arg2(a2),
            arg3()
    {}

    dynamic_instruction(const dynamic_instruction &di)
            :
            opcode(di.opcode),
            ip(di.ip),
            size(di.size),
            arg1(di.arg1),
            arg2(di.arg2),
            arg3(di.arg3)
    {
    }

    int opcode;
    int size;
    Int ip;
    dynamic_argument arg1;
    dynamic_argument arg2;
    dynamic_argument arg3;
};

struct internal_label {
    internal_label()
            :
            id(-1),
            address(-1)
    {}

    internal_label(Int id)
    :
        id(id),
        address(-1)
    {}

    internal_label(const internal_label &label)
            :
            id(label.id),
            address(label.address)
    {}

    Int id;
    Int address;
};

struct code_context {
    code_context()
            :
            ci(NULL),
            container(NULL),
            lineTable(),
            dynamicInstructions(),
            instructions(),
            tryCatchTable(),
            registers()
    {}

    code_context(const code_context &cc)
            :
            ci(cc.ci),
            instructions(cc.instructions),
            container(cc.container),
            lineTable(cc.lineTable),
            tryCatchTable(cc.tryCatchTable),
            dynamicInstructions(cc.dynamicInstructions),
            registers(cc.registers)
    {
    }

    ~code_context() {
        free();
    }

    void free();

    code_info* ci;
    List<try_catch_data*> tryCatchTable;
    List<internal_register*> registers;
    List<line_info*> lineTable;
    List<dynamic_instruction*> dynamicInstructions;
    List<internal_label*> labels;
    List<opcode_instr> instructions;
    sharp_function *container;
    machine_data machineData;
};

void set_machine_data(internal_register *internalRegister, bool force = false);
void set_machine_data(sharp_field *field, bool localField, bool force = false);
void set_machine_data(machine_data_type dataType, Int dataAddr, bool force = false);
void set_machine_data(machine_data_type dataType);
void clear_machine_data();
void push_machine_data_to_stack();
void pop_machine_data_from_stack();
void consume_machine_data();
void consume_machine_data(internal_register *);
void consume_machine_data_from_register(internal_register *internalRegister);
void increment_machine_data(data_type);
void decrement_machine_data(data_type);
void cast_machine_data(_register r, data_type type);
void create_dynamic_instruction(dynamic_instruction di);
void resolve_dynamic_instruction(List<opcode_instr> &instructions, dynamic_instruction &di);
void resolve_dynamic_instructions();
opcode_arg get_dynamic_arg_value(dynamic_argument &da);
internal_label* find_label(Int id);
void create_label(Int id);
Int create_label();
void set_label(internal_label *l);
void create_try_catch_data(Int id);
try_catch_data* find_try_catch_data(Int id);
void create_catch_data(Int id, Int tcd);
catch_data* find_catch_data(Int id, try_catch_data *tcd);
void create_finally_data(Int id, Int tcd);
finally_data* get_finally_data(Int id, try_catch_data *tcd);

Int create_constant(double constant);
Int create_constant(string &constant);

void update_context(code_info* ci, sharp_function *container);
void flush_context();

extern code_context cc;

#endif //SHARP_CODE_CONTEXT_H
