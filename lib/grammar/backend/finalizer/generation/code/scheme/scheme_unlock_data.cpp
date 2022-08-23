//
// Created by bknun on 8/22/2022.
//

#include "scheme_unlock_data.h"
#include "step_processor.h"
#include "scheme_processor.h"
#include "scheme_master.h"

void process_unlock_data_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_unlock(next_step);

    if(has_next_step)
        process_master_scheme(next_step->scheme);
}
