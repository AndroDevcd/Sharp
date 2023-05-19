//
// Created by bknun on 8/24/2022.
//

#include "scheme_lock.h"
#include "scheme_processor.h"
#include "step_processor.h"
#include "scheme_master.h"

void process_lock_scheme(operation_schema *scheme) {
    Int stepPos = 0;

    process_lock(next_step);
    process_master_scheme(next_step->scheme);
    process_unlock(next_step);
}
