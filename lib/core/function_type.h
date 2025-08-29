//
// Created by BNunnally on 9/19/2021.
//

#ifndef SHARP_FUNCTION_TYPE_H
#define SHARP_FUNCTION_TYPE_H

enum function_type {
    normal_function = 1,
    constructor_function = 2,
    initializer_function = 3,
    operator_function = 4,
    delegate_function = 5,
    blueprint_function = 6,
    native_function = 7,
    undefined_function = 0
};

#endif //SHARP_FUNCTION_TYPE_H
