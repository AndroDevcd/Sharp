//
// Created by bknun on 7/2/2023.
//

#include "while_step_runner.h"
#include "../optimizer.h"
#include "../../../types/sharp_function.h"
#include "../../generation/code/scheme/scheme_processor.h"
#include "variable_step_runner.h"


bool has_while_scheme_step(operation_schema *schema) {
    if(schema == NULL) {
        schema = octx.subject->scheme;

        auto recompiled = false;
        for(Int i = 0; i < schema->steps.size(); i++) {
            auto step = schema->steps.get(i);
            if(step->scheme && has_while_scheme_step(step->scheme)) {
                // process scheme
                recompile_while_scheme(step->scheme, schema);
                recompiled = true;
            }
        }

        return recompiled;
    }

    if(schema->schemeType == scheme_while)
        return true;
    else if(!schema->steps.empty()) {
        for(Int i = 0; i < schema->steps.size(); i++) {
            auto step = schema->steps.get(i);
            if(step->scheme && has_while_scheme_step(step->scheme)) {
                // process scheme
                recompile_while_scheme(step->scheme, schema);
            }
        }
    }

    return false;
}

void run_code_motion_while_steps() {
    has_while_scheme_step(NULL);
    flush_octx();
}

void recompile_while_scheme(operation_schema *scheme, operation_schema *parent) {
    if(!octx.processedSchemes.find(scheme)) {
        octx.processedSchemes.add(scheme);

        auto masterScheme = scheme->steps.get(5)->scheme;
        validate_scheme_type(masterScheme, scheme_master);

        List<operation_schema *> schemes;
        locate_single_variable_assignments(masterScheme, schemes);
        remove_schemes(masterScheme, schemes);

        Int startPos = 0;
        for (Int i = 0; i < parent->steps.size(); i++) {
            auto step = parent->steps.get(i);
            if (step->scheme && step->scheme == scheme) {
                // process scheme
                startPos = i;
                break;
            }
        }

        for (Int i = 0; i < schemes.size(); i++) {
            auto step = new operation_step(schemes[i]);
            parent->steps.insert(startPos++, step);

            APPLY_TEMP_SCHEME_WITHOUT_INJECT(0,
               scheme_0.schemeType = scheme_unused_data;
               create_unused_data_operation(&scheme_0);
            )
            step = new operation_step(&scheme_0);
            parent->steps.insert(startPos++, step);
        }
    }
}

void remove_schemes(operation_schema *schema, List<operation_schema*> &schemes) {
    if(!schema->steps.empty()) {
        for(Int i = 0; i < schema->steps.size(); i++) {
            auto step = schema->steps.get(i);
            if(step->scheme) {
                // process scheme
                if(schemes.find(step->scheme)) {
                    schema->steps.removeAt(i);
                    i--;
                } else {
                    remove_schemes(step->scheme, schemes);
                }
            }
        }
    }
}
