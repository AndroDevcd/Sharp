//
// Created by bknun on 7/7/2023.
//

#ifndef SHARP_GET_VALUE_ANALYZER_H
#define SHARP_GET_VALUE_ANALYZER_H

#include "code_analyzer.h"

code_fragment *analyze_get_value(operation_schema *scheme);
code_fragment *analyze_negate_value(operation_schema *scheme);
code_fragment *analyze_inc_value(operation_schema *scheme);
code_fragment *analyze_dec_value(operation_schema *scheme);
code_fragment *analyze_not_value(operation_schema *scheme);

#endif //SHARP_GET_VALUE_ANALYZER_H
