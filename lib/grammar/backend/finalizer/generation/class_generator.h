//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_CLASS_GENERATOR_H
#define SHARP_CLASS_GENERATOR_H

#include "code/code_info.h"

void generate_address(sharp_class *sc);
void generate_class_addresses(sharp_class *sc);
code_info* get_or_initialize_code(sharp_class *sc);

#endif //SHARP_CLASS_GENERATOR_H
