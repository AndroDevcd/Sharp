//
// Created by bknun on 8/21/2022.
//

#include "scheme_unused_data.h"
#include "step_processor.h"

void process_unused_data_scheme(operation_schema *scheme) {
    process_unused_data(scheme->steps.get(0));
}
