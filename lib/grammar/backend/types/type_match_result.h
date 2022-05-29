//
// Created by BNunnally on 9/22/2021.
//

#ifndef SHARP_TYPE_MATCH_RESULT_H
#define SHARP_TYPE_MATCH_RESULT_H

enum type_match_result {
    no_match_found = 0x0,
    match_normal = 0x1,
    match_constructor =0x2,
    match_operator_overload = 0x4,
    match_initializer = 0x8,
    is_direct_match = 0x10,
    exclude_all = match_constructor | match_operator_overload | match_initializer,
    overload_only = match_constructor | match_initializer,
    constructor_only = match_operator_overload | match_initializer
};

#endif //SHARP_TYPE_MATCH_RESULT_H
