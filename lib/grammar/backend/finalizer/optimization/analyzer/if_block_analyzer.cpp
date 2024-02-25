//
// Created by bknun on 7/8/2023.
//

#include "if_block_analyzer.h"
#include "../../generation/code/scheme/step_processor.h"

code_fragment* analyze_single_if_block(operation_schema *scheme) {
    auto condStep = scheme->steps[1];
    auto blockStep = scheme->steps[4];

    validate_step_type(condStep, operation_get_value);
    validate_step_type(blockStep, operation_step_scheme);

    return new if_blk(scheme, require_non_null(analyze_code(condStep->scheme)),
                      require_non_null(analyze_code(blockStep->scheme)));
}

code_fragment *analyze_if_block(operation_schema *scheme) {
    if(scheme->schemeType == scheme_if_single)
        return analyze_single_if_block(scheme);

    return NULL;
}
