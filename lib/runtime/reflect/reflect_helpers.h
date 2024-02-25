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
sharp_function* locate_function(const char *);
sharp_class* locate_class_simple(const char *name);
sharp_class* locate_class_hashed(uint32_t hash);


// Member assignment
void assign_numeric_field(sharp_object* o, uInt index, double value);
void assign_numeric_class_field(sharp_object* o, double value);
void assign_string_field(sharp_object* o, string &value);
bool is_static_class(object*);

// Member reading
double read_numeric_value(sharp_object *o, Int index);
string read_string_value(sharp_object *o);

#endif //SHARP_REFLECT_HELPERS_H
