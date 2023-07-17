//
// Created by bknun on 7/7/2023.
//

#include "get_value_analyzer.h"

code_fragment *analyze_get_value(operation_schema *scheme) {
    if(scheme->schemeType == scheme_get_value) {
        return require_non_null(analyze_code(scheme->steps[0]->scheme));
    }

    return NULL;
}