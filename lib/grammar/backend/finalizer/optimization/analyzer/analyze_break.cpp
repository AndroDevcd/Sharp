//
// Created by bknun on 7/8/2023.
//

#include "analyze_break.h"

code_fragment *analyze_break(operation_schema *scheme) {
    if(scheme->schemeType == scheme_break) {
        return new break_loop(scheme);
    }

    return NULL;
}
