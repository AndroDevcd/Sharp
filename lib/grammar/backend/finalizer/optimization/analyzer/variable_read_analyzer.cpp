//
// Created by bknun on 7/9/2023.
//

#include "variable_read_analyzer.h"
#include "../../generation/code/scheme/step_processor.h"

code_fragment *analyze_local_variable_read(operation_schema *scheme) {
    if(scheme->schemeType == scheme_access_local_field) {
        List<code_fragment*> actions;
        auto variableStep = scheme->steps[1];
        validate_step_type(variableStep, operation_get_local_field_value);

        return new read_variable(
                scheme, true, variableStep->field, actions
        );
    }

    return NULL;
}

code_fragment *analyze_instance_variable_read(operation_schema *scheme) {
    if(scheme->schemeType == scheme_access_instance_field) {
        List<code_fragment*> actions;

        for(Int i = 0; i < scheme->steps.size(); i++) {
            auto step = scheme->steps[i];

            if(step->scheme) {
                actions.add(analyze_code(step->scheme));
            }
        }

        return new read_variable(
                scheme, true, scheme->field, actions
        );
    }

    return NULL;
}
