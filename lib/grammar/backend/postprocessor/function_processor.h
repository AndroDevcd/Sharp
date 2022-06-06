//
// Created by BNunnally on 9/11/2021.
//

#ifndef SHARP_FUNCTION_PROCESSOR_H
#define SHARP_FUNCTION_PROCESSOR_H

#include "../../frontend/parser/Ast.h"
#include "../types/sharp_function.h"

struct sharp_function;
struct sharp_class;

void process_function(sharp_class *, function_type, Ast*);
void process_function(sharp_class*, string &, function_type, Ast*);
void process_generic_clone_functions(sharp_class*, string &, function_type, Ast*);
void process_generic_extension_functions(sharp_class*, sharp_class*);
void process_extension_class(sharp_class *&with_class, bool isDelegate, string &name, Ast*);
void process_function_return_type(sharp_function *fun);
void process_function_parameters(List<sharp_field*> &params, sharp_class *owner, Ast *ast);

void validate_function_type(
        bool hardType,
        sharp_function *fun,
        sharp_type &type,
        operation_schema *scheme,
        Ast *ast);

#endif //SHARP_FUNCTION_PROCESSOR_H
