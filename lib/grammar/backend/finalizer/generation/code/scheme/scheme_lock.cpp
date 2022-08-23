//
// Created by bknun on 8/22/2022.
//

#include "scheme_lock.h"
#include "step_processor.h"

void process_lock_data_scheme(operation_schema *scheme) {
    process_lock(scheme->steps.get(0));
}
