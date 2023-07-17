//
// Created by bknun on 7/2/2023.
//

#ifndef SHARP_WHILE_STEP_RUNNER_H
#define SHARP_WHILE_STEP_RUNNER_H

#include "../../../operation/operation.h"

void run_code_motion_while_steps();
void recompile_while_scheme(operation_schema *scheme, operation_schema *parent);
void remove_schemes(operation_schema *scheme, List<operation_schema*> &schemes);

#endif //SHARP_WHILE_STEP_RUNNER_H
