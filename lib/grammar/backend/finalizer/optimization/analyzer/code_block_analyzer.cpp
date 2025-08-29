//
// Created by bknun on 7/4/2023.
//

#include "code_block_analyzer.h"

code_fragment *analyze_code_block(operation_schema *scheme) {
    if(scheme->schemeType == scheme_master) {
        code_block *frag = new code_block(scheme);
        for(Int i = 0; i < scheme->steps.size(); i++) {
            auto step = scheme->steps[i];

            if(step->type == operation_step_scheme) {
                if(!is_ignored_scheme(step->scheme)) {
                    frag->nodes.add(require_non_null(analyze_code(step->scheme)));
                }
            } else {
                // do something else??? '\ ( -.- ) /'
            }
        }

        return frag;
    }

    return NULL;
}