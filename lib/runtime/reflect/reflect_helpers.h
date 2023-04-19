//
// Created by bknun on 9/21/2022.
//

#ifndef SHARP_REFLECT_HELPERS_H
#define SHARP_REFLECT_HELPERS_H

#include "../memory/sharp_object.h"
#include "../types/sharp_class.h"

// Member resolution
object* resolve_field(string name, sharp_object* o);

// Member Location
sharp_class* locate_class(const char *);


// Member assignment
void assign_number_field(sharp_object* o, double value);

bool is_static_class(object*);

#endif //SHARP_REFLECT_HELPERS_H
