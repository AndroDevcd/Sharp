//
// The declaration of a generic function, kept until it is used with concrete
// types. See compiler/functions/generic_functions.h.
//

#ifndef SHARP_GENERIC_FUNCTION_BLUEPRINT_H
#define SHARP_GENERIC_FUNCTION_BLUEPRINT_H

#include <string>
#include <vector>
#include "../../../core/function_type.h"

class Ast;
struct sharp_file;
struct sharp_module;

struct generic_function_blueprint {
    std::string name;                       // `map` in  def map<T, R>(...)
    std::vector<std::string> typeNames;     // T, R
    Ast *ast = nullptr;                     // the whole declaration (shared by every instance, never copied)
    function_type type = normal_function;   // normal_function or operator_function
    sharp_file *file = nullptr;             // the file and module that were current when it was declared:
    sharp_module *module = nullptr;         // an instance is processed and compiled as if it stood there
};

#endif //SHARP_GENERIC_FUNCTION_BLUEPRINT_H
