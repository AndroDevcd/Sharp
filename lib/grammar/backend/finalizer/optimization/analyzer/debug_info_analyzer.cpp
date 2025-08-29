//
// Created by bknun on 7/4/2023.
//

#include "debug_info_analyzer.h"

code_fragment *analyze_debug_info(operation_schema *scheme) {
    if(scheme->schemeType == scheme_line_info) {
        return new debugging_info(scheme);
    }

    return NULL;
}
