//
// Created by bknun on 7/5/2023.
//

#include "new_class_analyzer.h"
#include "../../generation/code/scheme/step_processor.h"

code_fragment *analyze_new_class(operation_schema *scheme) {
    if(scheme->schemeType == scheme_new_class) {
        auto classStep = scheme->steps[0];
        auto constructorStep = scheme->steps[2];

        validate_step_type(classStep, operation_create_class);
        validate_step_type(constructorStep, operation_step_scheme);

        auto sc = classStep->_class;
        auto constr = constructorStep->scheme->fun;
        List<code_fragment*> params;

        for(Int i = 0; i < constructorStep->scheme->steps.size(); i++) {
            auto paramStep = constructorStep->scheme->steps[i];
            if(paramStep->type == operation_push_parameter_to_stack) {
                params.add(require_non_null(analyze_code(paramStep->scheme)));
            }
        }

        return new new_class(scheme, sc, constr, params);
    }

    return NULL;
}
