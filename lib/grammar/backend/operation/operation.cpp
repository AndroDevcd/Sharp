//
// Created by BNunnally on 9/9/2021.
//

#include "operation.h"
#include "../types/sharp_field.h"

void create_local_field_access_operation(
        operation_scheme *scheme,
        sharp_field *localField) {
    if(scheme) {
        scheme->schemeType = scheme_access_local_field;
        scheme->field = localField;

        scheme->steps.add(operation_step(
                operation_get_local_field_value, localField));
    }
}

void create_static_field_access_operation(
        operation_scheme *scheme,
        sharp_field *staticField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_access_static_field;
        scheme->field = staticField;

        scheme->steps.add(operation_step(
                operation_get_static_class_instance, staticField->owner));
        scheme->steps.add(operation_step(
                operation_get_instance_field_value, staticField));
    }
}

void create_primary_instance_field_access_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_access_instance_field;
        scheme->field = instanceField;

        scheme->steps.add(operation_step(
                operation_get_primary_class_instance, instanceField->owner));
        scheme->steps.add(operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_primary_instance_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_call_getter_function;
        scheme->field = instanceField;

        scheme->steps.add(operation_step(
                operation_get_primary_class_instance, instanceField->owner));
        scheme->steps.add(operation_step(
                operation_get_instance_field_value, instanceField));
    }
}

void create_instance_field_access_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_access_instance_field;
        scheme->field = instanceField;

        scheme->steps.add(operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_instance_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_call_getter_function;
        scheme->field = instanceField;

        scheme->steps.add(operation_step(
                operation_get_instance_field_value, instanceField));
    }
}

void create_get_static_function_address_operation(
        operation_scheme *scheme,
        sharp_function *fun) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_get_address;

        scheme->steps.add(operation_step(
                operation_get_static_function_address, fun));
    }
}

void operation_step::freeStep()  {
    if(scheme) delete scheme;
}

void operation_scheme::copy(const operation_scheme &scheme) {
    schemeType = scheme.schemeType;
    field = scheme.field;
    steps.addAll(scheme.steps);
}
