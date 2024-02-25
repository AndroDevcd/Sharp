//
// Created by bknun on 8/8/2022.
//

#ifndef SHARP_GENERATOR_H
#define SHARP_GENERATOR_H

#include "../../../../../stdimports.h"
#include "../../types/types.h"

extern Int UUIDGenerator;
void generate();
void pre_generate_addresses();
void generation_error(string message);

extern List<sharp_function*> compressedCompilationFunctions;
extern List<sharp_class*> compressedCompilationClasses;
extern List<double> constantMap;
extern List<string> stringMap;

#endif //SHARP_GENERATOR_H
