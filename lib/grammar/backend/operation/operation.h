//
// Created by BNunnally on 9/9/2021.
//

#ifndef SHARP_OPERATION_H
#define SHARP_OPERATION_H

#include "../../../../stdimports.h"
#include "../../List.h"
#include "../types/type_match_result.h"

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
    operation_push_parameter_to_stack,
    operation_create_class
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
    scheme_new_class
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

    void free() {
        steps.free();
    }

    _operation_scheme schemeType;
    sharp_field *field;
    sharp_function *fun;
    sharp_class *sc;
    List<operation_step> steps;
};

struct operation_step {
    operation_step()
    :
        type(operation_none),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL)
    {}

    operation_step(const operation_step &step)
    :
        type(step.type),
        field(step.field),
        _class(step._class),
        scheme(new operation_scheme(*step.scheme)),
        function(step.function)
    {}

    operation_step(operation_type type)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(NULL)
    {}

    operation_step(operation_scheme *scheme, operation_type type = operation_step_scheme)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(scheme),
        function(NULL)
    {}

    operation_step(operation_type type, sharp_field *field)
    :
        type(type),
        field(field),
        _class(NULL),
        scheme(NULL),
        function(NULL)
    {}

    operation_step(operation_type type, sharp_function *fun)
    :
        type(type),
        field(NULL),
        _class(NULL),
        scheme(NULL),
        function(fun)
    {}

    operation_step(operation_type type, sharp_class *sc)
    :
        type(type),
        field(NULL),
        _class(sc),
        scheme(NULL),
        function(NULL)
    {}

    ~operation_step() {
        freeStep();
    }

    void freeStep();

    operation_scheme *scheme;
    sharp_field *field;
    sharp_class *_class;
    sharp_function *function;
    operation_type type;
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
        List<operation_scheme> &paramScheme,
        sharp_function *fun);

void create_primary_class_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme> &paramScheme,
        sharp_function *fun);

void create_static_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme> &paramScheme,
        sharp_function *fun);

void create_function_parameter_push_operation(
        sharp_type *paramType,
        type_match_result matchResult,
        sharp_function *constructor,
        operation_scheme *paramScheme,
        operation_scheme *resultScheme);


#endif //SHARP_OPERATION_H
