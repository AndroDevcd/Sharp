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
struct sharp_label;
struct sharp_tc_data;
struct catch_data;
struct finally_data;

enum operation_type {
    operation_none,
    operation_step_scheme,
    operation_get_local_field_value,
    operation_get_instance_field_value,
    operation_get_static_class_instance,
    operation_get_primary_class_instance,
    operation_get_tls_field_value,
    operation_call_instance_function,
    operation_call_static_function,
    operation_call_dynamic_function,
    operation_get_static_function_address,
    operation_push_value_to_stack,
    operation_post_scheme_start,
    operation_assign_array_value,
    operation_get_value,
    operation_post_access,
    operation_get_sizeof,
    operation_throw_exception,
    operation_pop_value_from_stack,
    operation_unused_data,
    operation_assign_array_element_from_stack,
    operation_nullify_value,
    operation_push_parameter_to_stack,
    operation_get_integer_constant,
    operation_get_decimal_constant,
    operation_negate_value,
    operation_create_class,
    operation_duplicate_item,
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
    operation_is_var,
    operation_get_array_element_at_index,
    operation_retain_numeric_value, // will try to store in register if possible
    operation_discard_register,
    operation_allocate_register,
    operation_allocate_label,
    operation_allocate_try_catch_data,
    operation_allocate_catch_data,
    operation_allocate_finally_data,
    operation_set_catch_field,
    operation_set_finally_exception_field,
    operation_set_catch_start,
    operation_set_finally_start,
    operation_set_finally_end,
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
    operation_assign_numeric_value,
    operation_record_line,
    operation_return_nil,
    operation_return_with_error_state,
    operation_return_number,
    operation_return_object,
    operation_lock,
    operation_unlock,
    operation_set_label,
    operation_jump_if_false,
    operation_jump_if_true,
    operation_jump,
    operation_label_reachable,
    operation_setup_local_field,
    operation_get_register_value,
    operation_set_register_value,
    operation_set_try_catch_start,
    operation_set_try_catch_end,
    operation_set_try_catch_block_start,
    operation_set_try_catch_block_end,
    operation_check_null,
    operation_no_op,
};

enum _operation_scheme {
    scheme_none,
    scheme_access_primary_instance_field,
    scheme_access_instance_field,
    scheme_access_static_field,
    scheme_access_tls_field,
    scheme_negate_value,
    scheme_inc_value,
    scheme_dec_value,
    scheme_not_value,
    scheme_master,
    scheme_return,
    scheme_get_constant,
    scheme_line_info,
    scheme_call_primary_class_instance_function,
    scheme_call_instance_function,
    scheme_call_dynamic_function,
    scheme_call_static_function,
    scheme_get_primary_class_instance,
    scheme_access_local_field,
    scheme_new_class_array,
    scheme_new_number_array,
    scheme_new_object_array,
    scheme_nullify_value,
    scheme_new_class,
    scheme_assign_array_value,
    scheme_get_size_of,
    scheme_get_casted_value,
    scheme_check_type,
    scheme_null_fallback,
    scheme_inline_if,
    scheme_assign_value,
    scheme_compound_assign_value,
    scheme_binary_math,
    scheme_accelerated_binary_math,
    scheme_or,
    scheme_null_check,
    scheme_instance_check,
    scheme_post_increment,
    scheme_unused_data,
    scheme_get_value,
    scheme_for,
    scheme_for_infinite,
    scheme_for_cond,
    scheme_for_iter,
    scheme_for_variable,
    scheme_lock_data,
    scheme_unlock_data,
    scheme_if_single,
    scheme_compound_if,
    scheme_increment_for_index_value,
    scheme_for_each,
    scheme_elseif,
    scheme_else,
    scheme_for_each_position_check,
    scheme_for_each_get_array_item,
    scheme_when,
    scheme_when_clause,
    scheme_when_else_clause,

    scheme_call_getter_function,
    scheme_get_address,
    scheme_label,
    scheme_while,
    scheme_throw,
    scheme_break,
    scheme_continue,
    scheme_lock,
    scheme_try,
};

struct operation_schema {
    operation_schema()
            :
            schemeType(scheme_none),
            field(NULL),
            sc(NULL),
            fun(NULL),
            steps(),
            type()
    {}

    operation_schema(_operation_scheme type)
            :
            schemeType(type),
            field(NULL),
            sc(NULL),
            fun(NULL),
            steps(),
            type()
    {}

    operation_schema(const operation_schema &scheme)
            :
            schemeType(scheme_none),
            field(NULL),
            sc(NULL),
            fun(NULL),
            steps(),
            type()
    {
        copy(scheme);
    }

    ~operation_schema() {
        free();
    }

    void copy(const operation_schema &scheme);

    void free();

    _operation_scheme schemeType;
    sharp_field *field;
    sharp_type type;
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

    operation_step(operation_schema *scheme, operation_type type = operation_step_scheme)
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
        this->scheme = new operation_schema(*scheme);
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

    operation_step(operation_type type, operation_schema *scheme)
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
        this->scheme = new operation_schema(*scheme);
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
            scheme = new operation_schema(*step.scheme);
    }

    void freeStep();

    operation_schema *scheme;
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
        operation_schema *scheme,
        sharp_field *localField);

void create_static_field_access_operation(
        operation_schema *scheme,
        sharp_field *staticField,
        bool resetState = true);

void create_primary_instance_field_access_operation(
        operation_schema *scheme,
        sharp_field *instanceField);

void create_tls_field_access_operation(
        operation_schema *scheme,
        sharp_field *tlsField,
        bool resetState = true);

void add_scheme_operation(
        operation_schema *scheme,
        operation_schema *valueScheme);

void create_primary_instance_field_getter_operation(
        operation_schema *scheme,
        sharp_field *instanceField);

void create_instance_field_access_operation(
        operation_schema *scheme,
        sharp_field *instanceField);

void create_instance_field_getter_operation(
        operation_schema *scheme,
        sharp_field *instanceField);

void create_get_static_function_address_operation(
        operation_schema *scheme,
        sharp_function *fun);

void create_instance_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun);

void create_primary_class_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun);

void create_static_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun);

void create_dynamic_function_call_operation(
        operation_schema *scheme,
        List<operation_schema*> &paramScheme,
        sharp_function *fun,
        Int registerId,
        bool resetState = true);

void create_function_parameter_push_operation(
        sharp_type *paramType,
        Int matchResult,
        sharp_function *constructor,
        operation_schema *paramScheme,
        operation_schema *resultScheme);

void create_get_integer_constant_operation(
        operation_schema *scheme,
        Int integer,
        bool resetState = true,
        bool setType = true);

void create_get_decimal_constant_operation(
        operation_schema *scheme,
        long double decimal,
        bool resetState = true,
        bool setType = true);

void create_get_char_constant_operation(
        operation_schema *scheme,
        char _char,
        bool resetState = true,
        bool setType = true);

void create_get_bool_constant_operation(
        operation_schema *scheme,
        bool _bool,
        bool resetState = true,
        bool setType = true);

void create_get_string_constant_operation(
        operation_schema *scheme,
        string &_string);

void create_value_assignment_operation(
        operation_schema *scheme,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_line_record_operation(
        operation_schema *scheme,
        Int line);

void create_negate_operation(operation_schema *scheme);

void create_not_operation(operation_schema *scheme);

void create_post_access_operation(operation_schema *scheme);

void create_increment_operation(
        operation_schema *scheme,
        data_type type);

void create_decrement_operation(
        operation_schema *scheme,
        data_type type);

void create_duplicate_operation(
        operation_schema *scheme);

void create_get_primary_instance_class(
        operation_schema *scheme,
        sharp_class *primaryClass);

void create_static_field_getter_operation(
        operation_schema *scheme,
        sharp_field *instanceField);

void create_null_value_operation(operation_schema *scheme);

void create_new_class_array_operation(
        operation_schema *scheme,
        operation_schema *arraySizeOperations,
        sharp_class *sc);

void create_new_number_array_operation(
        operation_schema *scheme,
        operation_schema *arraySizeOperations,
        data_type nativeType);

void create_new_object_array_operation(
        operation_schema *scheme,
        operation_schema *arraySizeOperations);

void create_push_to_stack_operation(
        operation_schema *scheme);

void create_post_scheme_start_operation(
        operation_schema *scheme);

void create_check_null_operation(
        operation_schema *scheme);

void create_retain_numeric_value_operation(
        operation_schema *scheme,
        Int registerId);

void create_get_value_from_register_operation(
        operation_schema *scheme,
        Int registerId);

void create_set_value_to_register_operation(
        operation_schema *scheme,
        Int registerId);

void create_and_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_xor_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_or_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_and_and_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_or_or_operation(
        operation_schema *scheme,
        operation_schema *leftScheme,
        operation_schema *rightScheme);

void create_eq_eq_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_not_eq_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_lt_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_gt_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_lte_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_shl_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_shr_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_add_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_sub_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_div_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_mod_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_exponent_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_mult_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_gte_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight);

void create_return_operation(
        operation_schema *scheme);

void create_return_with_error_operation(
        operation_schema *scheme);

void create_numeric_return_operation(
        operation_schema *scheme,
        operation_schema *valueScheme);

void create_object_return_operation(
        operation_schema *scheme,
        operation_schema *valueScheme);

void create_lock_operation(
        operation_schema *scheme,
        operation_schema *lockScheme);

void create_unlock_operation(
        operation_schema *scheme,
        operation_schema *lockScheme);

void create_instance_eq_operation(
        operation_schema *scheme);

void create_instance_not_eq_operation(
        operation_schema *scheme);

#define ALLOCATE_REGISTER_2X(r1, r2, scheme, code) \
            { Int register_##r1 = create_allocate_register_operation(scheme); \
            Int register_##r2 = create_allocate_register_operation(scheme); \
             code \
            create_deallocate_register_operation(scheme, register_##r1); \
            create_deallocate_register_operation(scheme, register_##r2); }

#define ALLOCATE_REGISTER_1X(r1, scheme, code) \
            { Int register_##r1 = create_allocate_register_operation(scheme); \
             code \
            create_deallocate_register_operation(scheme, register_##r1); }

#define APPLY_TEMP_SCHEME(scheme_num, scheme, code) \
            operation_schema scheme_##scheme_num; \
             {code}                                   \
            (scheme).steps.add(new operation_step(operation_step_scheme, &(scheme_##scheme_num)));

#define APPLY_TEMP_SCHEME_WITHOUT_INJECT(scheme_num, code) \
            operation_schema scheme_##scheme_num; \
             code

#define APPLY_TEMP_SCHEME_WITH_TYPE(scheme_num, scheme_type, scheme, code) \
             { operation_schema scheme_##scheme_num((scheme_type)); \
             code                                   \
            (scheme).steps.add(new operation_step(operation_step_scheme, &(scheme_##scheme_num))); }

void create_deallocate_register_operation(
        operation_schema *scheme,
        Int registerId);

Int create_allocate_register_operation(
        operation_schema *scheme);

Int create_allocate_label_operation(
        operation_schema *scheme);

Int create_allocate_try_catch_data_operation(
        operation_schema *scheme);

Int create_allocate_catch_data_operation(
        sharp_tc_data *parent,
        operation_schema *scheme);

Int create_allocate_finally_data_operation(
        sharp_tc_data *parent,
        operation_schema *scheme);

void create_set_catch_field_operation(
        catch_data *data,
        sharp_field *field,
        operation_schema *scheme);

void create_set_finally_field_operation(
        finally_data *data,
        sharp_field *field,
        operation_schema *scheme);

void create_set_catch_class_operation(
        catch_data *data,
        sharp_class *sc,
        operation_schema *scheme);

void create_set_label_operation(
        operation_schema *scheme,
        sharp_label* label);

void create_set_label_operation(
        operation_schema *scheme,
        Int label);

void create_catch_start_operation(
        operation_schema *scheme,
        catch_data* data);

void create_finally_start_operation(
        operation_schema *scheme,
        finally_data* data);

void create_finally_end_operation(
        operation_schema *scheme,
        finally_data* data);

void create_try_catch_start_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data);

void create_try_catch_block_start_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data);

void create_try_catch_end_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data);

void create_no_operation(
        operation_schema *scheme);

void create_try_catch_block_end_operation(
        operation_schema *scheme,
        sharp_tc_data* tc_data);

void create_jump_if_false_operation(
        operation_schema *scheme,
        sharp_label* label);

void create_jump_if_true_operation(
        operation_schema *scheme,
        sharp_label* label);

void create_jump_if_true_operation(
        operation_schema *scheme,
        Int label);

void create_setup_local_field_operation(
        operation_schema *scheme,
        sharp_field* field);

void create_jump_operation(
        operation_schema *scheme,
        sharp_label* label);

void create_get_value_operation(
        operation_schema *scheme,
        operation_schema *valueScheme,
        bool resetState = true,
        bool setType = true);

void create_plus_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_sub_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_mult_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_div_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_mod_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_and_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_or_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_xor_value_assignment_operation(
        operation_schema *scheme,
        Int registerLeft,
        Int registerRight,
        operation_schema *asigneeScheme,
        operation_schema *valueScheme);

void create_pop_value_from_stack_operation(
        operation_schema *scheme);

void create_unused_expression_data_operation(
        operation_schema *scheme);

void create_assign_array_element_operation(
        operation_schema *scheme,
        Int index,
        bool isNumeric);

void create_access_array_element_operation(
        operation_schema *scheme,
        sharp_type &type,
        operation_schema *indexScheme);

void create_new_class_operation(
        operation_schema *scheme,
        sharp_class *sc);

void create_sizeof_operation(
        operation_schema *scheme,
        operation_schema *valueOperation);

void create_throw_operation(
        operation_schema *scheme,
        operation_schema *valueOperation);

void create_cast_operation(
        operation_schema *scheme,
        sharp_type *cast_type);

void create_is_operation(
        operation_schema *scheme,
        sharp_type *cast_type);

void create_null_fallback_operation(
        operation_schema *scheme,
        operation_schema *nullScheme,
        operation_schema *fallbackScheme);

#endif //SHARP_OPERATION_H
