//
// Created by bknun on 4/26/2023.
//

#ifndef SHARP_CONSOLE_HELPER_H
#define SHARP_CONSOLE_HELPER_H

#include "../../stdimports.h"

void dbl_printf(int format, double value, int precision);

/**
 * A decimal as text, the way a person would write it: 5, 0.25, -3.5, 0.1.
 * THE one place the VM turns a number into digits for a program - printing
 * (DPUT), text + number, and to_string() all come here, so they agree.
 */
std::string natural_number(double value);

#endif //SHARP_CONSOLE_HELPER_H
