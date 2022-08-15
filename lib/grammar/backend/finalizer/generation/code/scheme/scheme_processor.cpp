//
// Created by bknun on 8/8/2022.
//

#include "scheme_processor.h"
#include "../code_context.h"
#include "../../generator.h"
#include "step_processor.h"
#include "scheme_access_static_field.h"
#include "scheme_access_primary_instance_field.h"
#include "scheme_access_tls_field.h"
#include "scheme_master.h"
#include "scheme_return.h"
#include "scheme_negate_value.h"
#include "scheme_line_info.h"
#include "scheme_inc_value.h"
#include "scheme_dec_value.h"
#include "scheme_not_value.h"
#include "scheme_call_static_function.h"
#include "scheme_get_constant.h"
#include "scheme_call_instance_function.h"
#include "scheme_new_class.h"
#include "scheme_call_primary_class_instance_function.h"
#include "scheme_call_dynamic_function.h"

void process_scheme(operation_schema *scheme) {
    if(scheme != NULL) {
        switch (scheme->schemeType) {
            case scheme_access_primary_instance_field:
                return process_access_primary_instance_field(scheme);
            case scheme_access_instance_field:
                return process_access_primary_instance_field(scheme);
            case scheme_access_static_field:
                return process_access_static_field_scheme(scheme);
            case scheme_access_tls_field:
                return process_access_tls_field_scheme(scheme);
            case scheme_master:
                return process_master_scheme(scheme);
            case scheme_return:
                return process_return_scheme(scheme);
            case scheme_negate_value:
                return process_negate_value_scheme(scheme);
            case scheme_line_info:
                return process_line_info_scheme(scheme);
            case scheme_inc_value:
                return process_increment_value_scheme(scheme);
                return process_increment_value_scheme(scheme);
            case scheme_dec_value:
                return process_decrement_value_scheme(scheme);
            case scheme_not_value:
                return process_not_value_scheme(scheme);
            case scheme_call_static_function:
                return process_call_static_function_scheme(scheme);
            case scheme_get_constant:
                return process_get_constant_scheme(scheme);
            case scheme_call_instance_function:
                return process_call_instance_function_scheme(scheme);
            case scheme_call_primary_class_instance_function:
                return process_primary_class_instance_function_scheme(scheme);
            case scheme_new_class:
                return process_new_class_scheme(scheme);
            case scheme_call_dynamic_function:
                return process_call_dynamic_function_scheme(scheme);
        }
    }
}

/**
 * A very simplistic way to process a scheme that dosen't require any additional reflection/logic
 * to ensure the correct coe is generated
 *
 * @param scheme
 */
void process_scheme_steps(operation_schema *scheme) {
    for(Int i = 0; i < scheme->steps.size(); i++) {
        process_step(scheme->steps.get(i));
    }
}

void process_scheme(operation_schema *scheme, code_info* ci, sharp_function *container) {
    update_context(ci, container);
    process_scheme(scheme);
    flush_context();
}
