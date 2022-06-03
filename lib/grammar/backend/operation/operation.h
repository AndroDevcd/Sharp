//
// Created by BNunnally on 9/9/2021.
//

#ifndef SHARP_OPERATION_H
#define SHARP_OPERATION_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../types/type_match_result.h"
#include "../types/sharp_type.h"

struct sharp_field;
struct sharp_class;
struct sharp_function;
struct operation_step;
struct sharp_type;

enum operation_type {
    operation_none,
    operation_step_scheme,
    operation_get_local_field_value,
    operation_get_instance_field_value,
    operation_get_static_class_instance,
    operation_get_primary_class_instance,
    operation_call_instance_function,
    operation_call_static_function,
    operation_get_static_function_address,
    operation_push_value_to_stack,
    operation_assign_array_value,
    operation_get_value,
    operation_get_sizeof,
    operation_pop_value_from_stack,
    operation_assign_array_element_from_stack,
    operation_nullify_value,
    operation_push_parameter_to_stack,
    operation_get_integer_constant,
    operation_get_decimal_constant,
    operation_negate_value,
    operation_create_class,
    operation_int8_increment,
    operation_int16_increment,
    operation_int32_increment,
    operation_int64_increment,
    operation_uint8_increment,
    operation_uint16_increment,
    operation_uint32_increment,
    operation_uint64_increment,
    operation_illegal,
    operation_var_increment,
    operation_int8_decrement,
    operation_int16_decrement,
    operation_int32_decrement,
    operation_int64_decrement,
    operation_uint8_decrement,
    operation_uint16_decrement,
    operation_uint32_decrement,
    operation_uint64_decrement,
    operation_var_decrement,
    operation_get_char_constant,
    operation_get_bool_constant,
    operation_get_string_constant,
    operation_not_value,
    operation_create_class_array,
    operation_create_number_array,
    operation_create_object_array,
    operation_get_size_value,
    operation_cast_class,
    operation_int8_cast,
    operation_int16_cast,
    operation_int32_cast,
    operation_int64_cast,
    operation_uint8_cast,
    operation_uint16_cast,
    operation_uint32_cast,
    operation_uint64_cast,
    operation_is_class,
    operation_is_int8,
    operation_is_int16,
    operation_is_int32,
    operation_is_int64,
    operation_is_uint8,
    operation_is_uint16,
    operation_is_uint32,
    operation_is_uint64,
    operation_is_fun_ptr,
    operation_get_array_element_at_index,
    operation_get_value_if_null,
    operation_retain_numeric_value, // will try to store in register if possible
    operation_discard_register,
    operation_allocate_register,
    operation_and,
    operation_xor,
    operation_or,
    operation_and_and,
    operation_or_or,
    operation_eq_eq,
    operation_not_eq,
    operation_instance_eq,
    operation_instance_not_eq,
    operation_gt,
    operation_lt,
    operation_gte,
    operation_lte,
    operation_shr,
    operation_shl,
    operation_add,
    operation_sub,
    operation_div,
    operation_mod,
    operation_mult,
    operation_exponent,
    operation_assign_value,
    operation_record_line
};

enum _operation_scheme {
    scheme_none,
    scheme_access_instance_field,
    scheme_access_static_field,
    scheme_access_tls_field,
    scheme_access_local_field,
    scheme_call_getter_function,
    scheme_get_address,
    scheme_call_instance_function,
    scheme_call_static_function,
    scheme_get_constant,
    scheme_new_class,
    scheme_get_size_of,
    scheme_nullify_value,
    scheme_new_class_array,
    scheme_new_number_array,
    scheme_new_object_array,
    scheme_get_array_value,
    scheme_get_casted_value,
    scheme_get_primary_class_instance,
    scheme_check_type,
    scheme_null_fallback,
    scheme_assign_value,
    scheme_get_numeric_value,
    scheme_line_info
};

struct operation_scheme {
    operation_scheme()
            :
            schemeType(scheme_none),
            field(NULL),
            sc(NULL),
            fun(NULL),
            steps()
    {}

    operation_scheme(const operation_scheme &scheme)
            :
            schemeType(scheme_none),
            field(NULL),
            sc(NULL),
            fun(NULL),
            steps()
    {
        copy(scheme);
    }

    ~operation_scheme() {
        free();
    }

    void copy(const operation_scheme &scheme);

    void free();

    _operation_scheme schemeType;
    sharp_field *field;
    sharp_function *fun;
    sharp_class *sc;
    List<operation_step*> steps;
};

struct operation_step {
    operation_step()
    :
        type(operation_none),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(const operation_step &step)
    :
            type(operation_none),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            decimal(0),
            integer(0),
            secondRegister(0),
            thirdRegister(0),
            _char(0),
            _bool(false),
            _string(""),
            nativeType(type_undefined)
    {
        copy(step);
    }

    operation_step(operation_type type)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(operation_scheme *scheme, operation_type type = operation_step_scheme)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {
        this->scheme = new operation_scheme(*scheme);
    }

    operation_step(operation_type type, sharp_field *field)
    :
        type(type),
        field(field),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(operation_type type, Int constant)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        integer(constant),
        decimal(0),
        _char(0),
        secondRegister(0),
        thirdRegister(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(operation_type type, Int leftRegister, Int rightRegister)
            :
            type(type),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            integer(leftRegister),
            decimal(0),
            _char(0),
            secondRegister(rightRegister),
            thirdRegister(0),
            _bool(false),
            _string(""),
            nativeType(type_undefined)
    {}

    operation_step(operation_type type, Int r1, Int r2, Int r3)
            :
            type(type),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            integer(r1),
            decimal(0),
            _char(0),
            secondRegister(r2),
            thirdRegister(r3),
            _bool(false),
            _string(""),
            nativeType(type_undefined)
    {}

    operation_step(operation_type type, data_type nt)
            :
            type(type),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            integer(0),
            decimal(0),
            secondRegister(0),
            thirdRegister(0),
            _char(0),
            _bool(false),
            _string(""),
            nativeType(nt)
    {}

    operation_step(operation_type type, char constant)
            :
            type(type),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            integer(0),
            decimal(0),
            secondRegister(0),
            thirdRegister(0),
            _char(constant),
            _bool(false),
            _string(""),
            nativeType(type_undefined)
    {}

    operation_step(operation_type type, bool constant)
            :
            type(type),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            integer(0),
            decimal(0),
            secondRegister(0),
            thirdRegister(0),
            _char(0),
            _bool(constant),
            _string(""),
            nativeType(type_undefined)
    {}

    operation_step(operation_type type, string &constant)
            :
            type(type),
            field(NULL),
            _class(NULL),
            scheme(NULL),
            function(NULL),
            integer(0),
            decimal(0),
            _char(0),
            secondRegister(0),
            thirdRegister(0),
            _bool(false),
            _string(constant),
            nativeType(type_undefined)
    {}

    operation_step(operation_type type, long double constant)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        decimal(constant),
        integer(0),
        _char(0),
        secondRegister(0),
        thirdRegister(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(operation_type type, sharp_function *fun)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(fun),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(operation_type type, sharp_class *sc)
    :
        type(type),
        field(NULL),
        _class(sc),
        scheme(NULL),
        function(NULL),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {}

    operation_step(operation_type type, operation_scheme *scheme)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL),
        decimal(0),
        integer(0),
        secondRegister(0),
        thirdRegister(0),
        _char(0),
        _bool(false),
        _string(""),
        nativeType(type_undefined)
    {
        this->scheme = new operation_scheme(*scheme);
    }

    ~operation_step() {
        freeStep();
    }

    void copy(const operation_step &step) {
        type = step.type;
        _string = step._string;
        _bool = step._bool;
        _char = step._char;
        decimal = step.decimal;
        integer = step.integer;
        function = step.function;
        _class = step._class;
        field = step.field;
        nativeType = step.nativeType;
        secondRegister = step.secondRegister;
        thirdRegister = step.thirdRegister;

        if(step.scheme)
            scheme = new operation_scheme(*step.scheme);
    }

    void freeStep();

    operation_scheme *scheme;
    sharp_field *field;
    sharp_class *_class;
    sharp_function *function;
    Int integer;
    long double decimal;
    char _char;
    bool _bool;
    data_type nativeType;
    string _string;
    operation_type type;
    Int secondRegister;
    Int thirdRegister;
};

void create_local_field_access_operation(
        operation_scheme *scheme,
        sharp_field *localField);

void create_static_field_access_operation(
        operation_scheme *scheme,
        sharp_field *staticField);

void create_primary_instance_field_access_operation(
        operation_scheme *scheme,
        sharp_field *instanceField);

void create_primary_instance_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField);

void create_instance_field_access_operation(
        operation_scheme *scheme,
        sharp_field *instanceField);

void create_instance_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField);

void create_get_static_function_address_operation(
        operation_scheme *scheme,
        sharp_function *fun);

void create_instance_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme*> &paramScheme,
        sharp_function *fun);

void create_primary_class_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme*> &paramScheme,
        sharp_function *fun);

void create_static_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme*> &paramScheme,
        sharp_function *fun);

void create_function_parameter_push_operation(
        sharp_type *paramType,
        Int matchResult,
        sharp_function *constructor,
        operation_scheme *paramScheme,
        operation_scheme *resultScheme);

void create_get_integer_constant_operation(
        operation_scheme *scheme,
        Int integer,
        bool resetState = true,
        bool setType = true);

void create_get_decimal_constant_operation(
        operation_scheme *scheme,
        long double decimal);

void create_get_char_constant_operation(
        operation_scheme *scheme,
        char _char);

void create_get_bool_constant_operation(
        operation_scheme *scheme,
        bool _bool);

void create_get_string_constant_operation(
        operation_scheme *scheme,
        string &_string);

void create_value_assignment_operation(
        operation_scheme *scheme,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_line_record_operation(
        operation_scheme *scheme,
        Int line);

void create_negate_operation(operation_scheme *scheme);

void create_not_operation(operation_scheme *scheme);

void create_increment_operation(
        operation_scheme *scheme,
        data_type type);

void create_decrement_operation(
        operation_scheme *scheme,
        data_type type);

void create_get_primary_instance_class(
        operation_scheme *scheme,
        sharp_class *primaryClass);

void create_static_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField);

void create_null_value_operation(operation_scheme *scheme);

void create_new_class_array_operation(
        operation_scheme *scheme,
        operation_scheme *arraySizeOperations,
        sharp_class *sc);

void create_new_number_array_operation(
        operation_scheme *scheme,
        operation_scheme *arraySizeOperations,
        data_type nativeType);

void create_new_object_array_operation(
        operation_scheme *scheme,
        operation_scheme *arraySizeOperations);

void create_push_to_stack_operation(
        operation_scheme *scheme);

void create_retain_numeric_value_operation(
        operation_scheme *scheme,
        Int registerId);

void create_and_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_xor_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_or_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_and_and_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_or_or_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_eq_eq_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_not_eq_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_lt_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_gt_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_lte_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_shl_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_shr_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_add_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_sub_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_div_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_mod_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_exponent_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_mult_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_gte_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight);

void create_instance_eq_operation(
        operation_scheme *scheme);

void create_instance_not_eq_operation(
        operation_scheme *scheme);

#define ALLOCATE_REGISTER_2X(r1, r2, scheme, code) \
            Int register_##r1 = create_allocate_register_operation(scheme); \
            Int register_##r2 = create_allocate_register_operation(scheme); \
             code \
            create_deallocate_register_operation(scheme, register_##r1); \
            create_deallocate_register_operation(scheme, register_##r2);

#define APPLY_TEMP_SCHEME(scheme_num, scheme, code) \
            operation_scheme scheme_##scheme_num; \
             code                                   \
            scheme.add(new operation_step(operation_step_scheme, &(scheme_##scheme_num)));
void create_deallocate_register_operation(
        operation_scheme *scheme,
        Int registerId);

Int create_allocate_register_operation(
        operation_scheme *scheme);

void create_get_value_operation(
        operation_scheme *scheme,
        operation_scheme *valueScheme,
        bool resetState = true,
        bool setType = true);

void create_plus_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_sub_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_mult_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_div_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_mod_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_and_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_or_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_xor_value_assignment_operation(
        operation_scheme *scheme,
        Int registerLeft,
        Int registerRight,
        operation_scheme *asigneeScheme,
        operation_scheme *valueScheme);

void create_pop_value_from_stack_operation(
        operation_scheme *scheme);

void create_assign_array_element_operation(
        operation_scheme *scheme,
        Int index);

void create_access_array_element_operation(
        operation_scheme *scheme,
        operation_scheme *indexScheme);

void create_new_class_operation(
        operation_scheme *scheme,
        sharp_class *sc);

void create_sizeof_operation(
        operation_scheme *scheme,
        operation_scheme *valueOperation);

void create_cast_operation(
        operation_scheme *scheme,
        sharp_type *cast_type);

void create_is_operation(
        operation_scheme *scheme,
        sharp_type *cast_type);

void create_null_fallback_operation(
        operation_scheme *scheme,
        operation_scheme *nullScheme,
        operation_scheme *fallbackScheme);

#endif //SHARP_OPERATION_H
