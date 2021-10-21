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
    exclude_all = 0xe // excludes constructors, operator overloads, and initializers
};

#endif //SHARP_TYPE_MATCH_RESULT_H
