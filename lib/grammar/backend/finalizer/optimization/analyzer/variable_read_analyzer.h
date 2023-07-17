//
// Created by bknun on 7/9/2023.
//

#ifndef SHARP_VARIABLE_READ_ANALYZER_H
#define SHARP_VARIABLE_READ_ANALYZER_H

#include "code_analyzer.h"

code_fragment *analyze_local_variable_read(operation_schema *scheme);
code_fragment *analyze_instance_variable_read(operation_schema *scheme);

#endif //SHARP_VARIABLE_READ_ANALYZER_H
