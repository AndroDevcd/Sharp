//
// Created by bknun on 7/2/2022.
//

#include "try_catch_data.h"
#include "types.h"

sharp_tc_data* create_try_catch_data_tracker(
        operation_schema *scheme) {
    if(scheme) {
        return new sharp_tc_data(create_allocate_try_catch_data_operation(scheme));
    }

    return NULL;
}

catch_data_info* create_catch_data_tracker(
        sharp_tc_data *parent,
        operation_schema *scheme) {
    if(scheme) {
        return new catch_data_info(create_allocate_catch_data_operation(parent, scheme), parent);
    }

    return NULL;
}

finally_data_info* create_finally_data_tracker(
        sharp_tc_data *parent,
        operation_schema *scheme) {
    if(scheme) {
        return new finally_data_info(create_allocate_finally_data_operation(parent, scheme), parent);
    }
    
    return NULL;
}