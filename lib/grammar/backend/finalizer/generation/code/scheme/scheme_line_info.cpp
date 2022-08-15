//
// Created by bknun on 8/12/2022.
//

#include "scheme_line_info.h"
#include "step_processor.h"

void process_line_info_scheme(operation_schema *scheme) {
    process_record_line_info(scheme->steps.get(0));
}
