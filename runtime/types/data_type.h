//
// Created by bknun on 9/16/2022.
//

#ifndef SHARP_DATA_TYPE_H
#define SHARP_DATA_TYPE_H

enum data_type {
    type_int8=0,
    type_int16,
    type_int32,
    type_int64,
    type_uint8,
    type_uint16,
    type_uint32,
    type_uint64,
    // 8 was the function pointer when a function value was a number (the
    // index of the function). The slot stays so that decimal, object and class
    // keep the numbers that programs and the library know them by
    // (std/reflect/data_type.sharp).
    type_unused_8,
    type_var,
    type_object,
    type_class,
    // A function value: `fn : (int)(int)`. It sits AFTER the numbers on
    // purpose: every "is this a number" range check (type <= type_var) used
    // to count it as one. It is an object now - an instance of
    // platform.kernel#function_value, which holds the function's index and
    // the variables the function captured.
    // C++ equivalent: a std::function instead of a raw function pointer.
    type_function_ptr,
    type_field,
    type_function,
    type_lambda_function,
    type_module,
    type_import_group,
    type_null,
    type_nil,
    type_any,
    type_untyped,
    type_undefined,
    type_integer,
    type_char,
    type_string,
    type_bool,
    type_decimal,
    type_get_component_request,
    type_label,
};

#endif //SHARP_DATA_TYPE_H
