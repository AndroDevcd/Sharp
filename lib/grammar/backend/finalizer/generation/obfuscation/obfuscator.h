//
// Created by bknun on 11/12/2023.
//

#ifndef SHARP_OBFUSCATOR_H
#define SHARP_OBFUSCATOR_H

#include "../../../../../../stdimports.h"
#include "../../../../../util/KeyPair.h"
#include "../../../../../util/linked_list.h"
#include "../../../types/types.h"

sharp_module* obfuscate(sharp_module *subject);
sharp_field* obfuscate(sharp_field *subject);
sharp_file* obfuscate(sharp_file *subject);
sharp_class* obfuscate(sharp_class *subject);
sharp_function* obfuscate(sharp_function *subject);


#endif //SHARP_OBFUSCATOR_H
