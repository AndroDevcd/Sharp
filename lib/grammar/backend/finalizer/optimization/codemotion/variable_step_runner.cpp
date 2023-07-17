//
// Created by bknun on 7/3/2023.
//

#include "variable_step_runner.h"
#include "../optimizer.h"
#include "../../generation/code/scheme/scheme_processor.h"
#include "../../generation/code/scheme/step_processor.h"

sharp_field* get_assigned_variable(bool first, operation_schema *schema) {
    if(first) {
        validate_scheme_type(schema, scheme_assign_value);
        auto valueStep = schema->steps[2];
        validate_step_type(valueStep, operation_get_value);

        return get_assigned_variable(false, valueStep->scheme);
    } else {
        if (schema->schemeType == scheme_access_local_field) {
            return schema->field;
        } else if (!schema->steps.empty()) {
            for (Int i = 0; i < schema->steps.size(); i++) {
                auto step = schema->steps.get(i);
                sharp_field *field;
                if (step->scheme && (field = get_assigned_variable(false, step->scheme)) != NULL) {
                    // process scheme
                    return field;
                }
            }
        }

        return NULL;
    }

}

bool has_assign_variable_scheme_step(operation_schema *schema) {
    if(schema->schemeType == scheme_assign_value) {
        return true;
    }
    else if(!schema->steps.empty()) {
        for(Int i = 0; i < schema->steps.size(); i++) {
            auto step = schema->steps.get(i);
            if(step->scheme && has_assign_variable_scheme_step(step->scheme)) {
                // process scheme
                auto field = get_assigned_variable(true, step->scheme);
                if(field != NULL) {
                    add_variable_assignment(field, step->scheme);
                }
            }
        }
    }

    return false;
}

void locate_single_variable_assignments(operation_schema *scheme, List<operation_schema*> &schemes) {
    has_assign_variable_scheme_step(scheme);
    for(Int i = 0; i < octx.variableAssignments.size(); i++) {
        if(octx.variableAssignments[i]->assignSchemes.singular()) {
            schemes.add(octx.variableAssignments[i]->assignSchemes.first());
        }
    }
}