//
// Created by bknun on 8/9/2022.
//

#include "scheme_access_static_field.h"
#include "../code_context.h"
#include "step_processor.h"

void process_access_static_field_scheme(operation_schema *scheme) {
    process_get_static_class_instance(scheme->steps.get(0));
    process_get_instance_field_value(scheme->steps.get(1));
}

void process_access_primary_instance_field(operation_schema *scheme) {
    process_get_primary_class_instance(scheme->steps.get(0));
    process_get_instance_field_value(scheme->steps.get(1));
}