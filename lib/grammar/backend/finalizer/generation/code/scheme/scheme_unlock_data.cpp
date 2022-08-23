//
// Created by bknun on 8/22/2022.
//

#include "scheme_unlock_data.h"
#include "step_processor.h"

void process_unlock_data_scheme(operation_schema *scheme) {
    process_unlock(scheme->steps.get(0));
}
