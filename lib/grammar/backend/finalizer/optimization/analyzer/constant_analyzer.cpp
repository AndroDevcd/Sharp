//
// Created by bknun on 7/7/2023.
//

#include "constant_analyzer.h"

code_fragment *analyze_constant(operation_schema *scheme) {
    if(scheme->schemeType == scheme_get_constant) {
        return new get_constant_value(scheme);
    }

    return NULL;
}
