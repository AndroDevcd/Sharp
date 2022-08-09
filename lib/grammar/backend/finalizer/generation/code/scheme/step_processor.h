//
// Created by bknun on 8/9/2022.
//

#ifndef SHARP_STEP_PROCESSOR_H
#define SHARP_STEP_PROCESSOR_H

#include "../../../../operation/operation.h"

void process_get_static_class_instance(operation_step *step);
void process_get_instance_field_value(operation_step *step);
void process_get_primary_class_instance(operation_step *step);

#endif //SHARP_STEP_PROCESSOR_H
