//
// Created by bknun on 9/21/2022.
//

#ifndef SHARP_REFLECT_HELPERS_H
#define SHARP_REFLECT_HELPERS_H

#include "../memory/sharp_object.h"
#include "../types/sharp_class.h"

void ignore_static_resolve(bool ignore);

// Member resolution
object* resolve_field(string name, sharp_object* o);

// Member Location
sharp_class* locate_class(const char *);
sharp_function* locate_function(const char *);
sharp_class* locate_class_simple(const char *name);
sharp_class* locate_class_hashed(uint32_t hash);

// Class instantiation
object* instantiate_class_field(string &name, sharp_object *o, sharp_class *sc);
object* instantiate_class_field_array(string &name, sharp_object *o, sharp_class *sc, Int size);

// Member assignment
void assign_numeric_field(sharp_object* o, uInt index, double value);
void assign_object_field(sharp_object* o, uInt index, sharp_object* value);
void assign_string_field(sharp_object* o, string &value);
bool is_static_class(object*);

// Member reading
double read_numeric_value(sharp_object *o, Int index);
string read_string_value(sharp_object *o);

// Complex member Assignment
void assign_string_class_field(sharp_object* o, string &value);
void assign_numeric_class_field(sharp_object* o, double value);
void assign_object_class_field(sharp_object* o, string &name, sharp_object* value);
object* assign_instance_to_class_field(string &name, sharp_object* o, sharp_object *instance);

#endif //SHARP_REFLECT_HELPERS_H
