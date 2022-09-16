//
// Created by bknun on 8/11/2022.
//

#ifndef SHARP_FUNCTION_GENERATOR_H
#define SHARP_FUNCTION_GENERATOR_H

#include "code/code_info.h"

void generate_address(sharp_function *sf);
void generate(sharp_function *sf);
void generate_initial_closure_setup(sharp_function *sf);
void generate_static_class_setup();
void remove_delegate_functions();

#endif //SHARP_FUNCTION_GENERATOR_H
