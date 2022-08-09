//
// Created by bknun on 8/8/2022.
//

#include "scheme_processor.h"
#include "scheme_access_static_field.h"
#include "../code_context.h"

void process_scheme(operation_schema *scheme) {
    switch (scheme->schemeType) {
        case scheme_access_static_field:
            return process_access_static_field_scheme(scheme);
        case scheme_access_primary_instance_field:
            return process_access_primary_instance_field(scheme);
    }
}

void process_scheme(operation_schema *scheme, code_info* ci, sharp_function *container) {
    update_context(ci, container);
    process_scheme(scheme);
    flush_context();
}
