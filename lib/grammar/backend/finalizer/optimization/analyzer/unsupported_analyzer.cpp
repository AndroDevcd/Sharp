//
// Created by bknun on 7/9/2023.
//

#include "unsupported_analyzer.h"
#include "../../generation/code/scheme/step_processor.h"


code_fragment *analyze_unsupported_fragment(operation_schema *scheme) {
    auto ufrag = new unsupported(scheme);
    for(Int i = 0; i < scheme->steps.size(); i++) {
        auto step = scheme->steps[i];

        if(step->scheme != NULL) {
            ufrag->nodes.add(require_non_null(analyze_code(step->scheme)));
        }
    }

    return ufrag;
}