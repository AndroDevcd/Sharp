//
// Created by BNunnally on 9/22/2021.
//

#ifndef SHARP_TYPE_MATCH_RESULT_H
#define SHARP_TYPE_MATCH_RESULT_H

enum type_match_result {
    no_match_found = 0x0,
    direct_match = 0x1,
    indirect_match = 0x2,
    match_constructor =0x4,
    match_operator_overload = 0x8,
    match_initializer = 0x10,
    exclude_none = direct_match | indirect_match,
    exclude_all = match_constructor | match_operator_overload | match_initializer,
    overload_only = match_constructor | match_initializer,
    constructor_only = match_operator_overload | match_initializer
};

#define is_match_normal(match_result) (((match_result) == direct_match) || ((match_result) == indirect_match))

#endif //SHARP_TYPE_MATCH_RESULT_H
