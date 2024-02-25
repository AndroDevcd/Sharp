//
// Created by bknun on 7/9/2023.
//

#ifndef SHARP_SINGLE_SET_VARIABLE_PROCESSOR_H
#define SHARP_SINGLE_SET_VARIABLE_PROCESSOR_H

#include "../analyzer/code_analyzer.h"

/**
 *
 * @param variables
 * @param origin
 *
 * Look for variables that were set 1 time in a block of code
 *
 * example:
 * for(i := 0; i < 10; i++ {
 *   x := 9;
 * }
 */
void process_single_set_variables(List<write_variable*> &variables, code_fragment *origin);

#endif //SHARP_SINGLE_SET_VARIABLE_PROCESSOR_H
