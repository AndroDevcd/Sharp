//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_SCHEME_PROCESSOR_H
#define SHARP_SCHEME_PROCESSOR_H

#include "../../../../operation/operation.h"
#include "../code_info.h"

#define next_step (scheme->steps.get(stepPos++))
#define current_step (scheme->steps.get(stepPos))
#define has_next_step (stepPos < scheme->steps.size())

void process_scheme_steps(operation_schema *scheme);
void process_scheme(operation_schema *scheme, code_info* ci, sharp_function *container);
void process_scheme(operation_schema *scheme);
void validate_scheme_type(operation_schema *scheme, _operation_scheme type);

#endif //SHARP_SCHEME_PROCESSOR_H
