//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_FILE_GENERATOR_H
#define SHARP_FILE_GENERATOR_H

#include "../../../sharp_file.h"

void generate_addresses(sharp_file *file);
void generate_exe();

sharp_file* get_true_source_file(sharp_function *sf);

#endif //SHARP_FILE_GENERATOR_H
