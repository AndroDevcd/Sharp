//
// Created by bknun on 7/5/2023.
//

#include "variable_write_analyzer.h"
#include "../../generation/code/scheme/step_processor.h"

code_fragment *analyze_local_variable_write(operation_schema *scheme) {
    if(scheme->schemeType == scheme_assign_value) {
        auto variableStep = scheme->steps[2];
        auto variableValueStep = scheme->steps[0];
        validate_step_type(variableStep, operation_get_value);
        validate_step_type(variableValueStep, operation_get_value);

        if(variableStep->scheme && (variableStep->scheme->schemeType == scheme_access_local_field
            || variableStep->scheme->schemeType == scheme_access_primary_instance_field
            || variableStep->scheme->schemeType == scheme_access_instance_field
            || variableStep->scheme->schemeType == scheme_access_static_field
            || variableStep->scheme->schemeType == scheme_access_tls_field)) {
            return new write_variable(
                    scheme, true, variableStep->scheme->field,
                    require_non_null(analyze_code(variableValueStep->scheme))
            );
        }
    }

    return NULL;
}
