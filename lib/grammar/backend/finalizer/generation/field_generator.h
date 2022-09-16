//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_FIELD_GENERATOR_H
#define SHARP_FIELD_GENERATOR_H

#include "../../types/sharp_field.h"

extern uInt threadLocalCount;

void generate_address(sharp_field *field);
code_info* get_or_initialize_code(sharp_field *field);
void generate_address(sharp_field *field, Int localFieldIndex, bool staticFunction);

#endif //SHARP_FIELD_GENERATOR_H
