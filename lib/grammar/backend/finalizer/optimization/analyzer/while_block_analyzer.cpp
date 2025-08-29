//
// Created by bknun on 7/7/2023.
//

#include "while_block_analyzer.h"
#include "../../generation/code/scheme/step_processor.h"

code_fragment *analyze_while_block(operation_schema *scheme) {
    if(scheme->schemeType == scheme_while) {
        auto condStep = scheme->steps[3];
        auto blockStep = scheme->steps[5];
        validate_step_type(condStep, operation_get_value);
        validate_step_type(blockStep, operation_step_scheme);
        auto cond = require_non_null(analyze_code(condStep->scheme));
        auto block = require_non_null(analyze_code(blockStep->scheme));

        return new while_loop(scheme, cond, block);
    }

    return NULL;
}