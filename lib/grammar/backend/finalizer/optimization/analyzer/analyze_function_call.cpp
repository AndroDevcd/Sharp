//
// Created by bknun on 7/9/2023.
//

#include "analyze_function_call.h"

code_fragment *analyze_function_call(operation_schema *scheme) {
    if(scheme->schemeType == scheme_call_instance_function) {
        List<code_fragment*> actions;

        for(Int i = 0; i < scheme->steps.size(); i++) {
            auto step = scheme->steps[i];

            if(step->scheme) {
                actions.add(analyze_code(step->scheme));
            }
        }

        return new function_call(scheme, actions);
    }

    return NULL;
}