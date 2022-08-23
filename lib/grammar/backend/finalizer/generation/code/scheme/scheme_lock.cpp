//
// Created by bknun on 8/22/2022.
//

#include "scheme_lock.h"
#include "step_processor.h"
#include "scheme_processor.h"
#include "scheme_master.h"

void process_lock_data_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_lock(next_step);

    if(has_next_step)
        process_master_scheme(next_step->scheme);
}
