//
// Created by BNunnally on 9/9/2021.
//

#include "operation.h"
#include "../types/sharp_field.h"
#include "../types/sharp_function.h"

void create_local_field_access_operation(
        operation_scheme *scheme,
        sharp_field *localField) {
    if(scheme) {
        scheme->schemeType = scheme_access_local_field;
        scheme->field = localField;

        scheme->steps.add(new operation_step(
                operation_get_local_field_value, localField));
    }
}

void create_static_field_access_operation(
        operation_scheme *scheme,
        sharp_field *staticField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_access_static_field;
        scheme->field = staticField;

        scheme->steps.add(new operation_step(
                operation_get_static_class_instance, staticField->owner));
        scheme->steps.add(new operation_step(
                operation_get_instance_field_value, staticField));
    }
}

void create_primary_instance_field_access_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_access_instance_field;
        scheme->field = instanceField;

        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, instanceField->owner));
        scheme->steps.add(new operation_step(
                operation_get_instance_field_value, instanceField));
    }
}

void create_primary_instance_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_call_getter_function;
        scheme->fun = instanceField->getter;

        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, instanceField->owner));
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack));
        scheme->steps.add(new operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_static_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_call_getter_function;
        scheme->fun = instanceField->getter;

        scheme->steps.add(new operation_step(
                operation_get_static_class_instance, instanceField->owner));
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack));
        scheme->steps.add(new operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_new_class_array_operation(
        operation_scheme *scheme,
        operation_scheme *arraySizeOperations,
        sharp_class *sc) {
    if(scheme) {
        scheme->schemeType = scheme_new_class_array;

        scheme->steps.add(new operation_step(
                arraySizeOperations, operation_get_size_value));

        scheme->steps.add(new operation_step(
                operation_create_class_array, sc));
    }
}

void create_new_number_array_operation(
        operation_scheme *scheme,
        operation_scheme *arraySizeOperations,
        native_type nativeType) {
    if(scheme) {
        scheme->schemeType = scheme_new_number_array;

        scheme->steps.add(new operation_step(
                arraySizeOperations, operation_get_size_value));

        scheme->steps.add(new operation_step(
                operation_create_number_array, nativeType));
    }
}

void create_new_object_array_operation(
        operation_scheme *scheme,
        operation_scheme *arraySizeOperations) {
    if(scheme) {
        scheme->schemeType = scheme_new_object_array;

        scheme->steps.add(new operation_step(
                arraySizeOperations, operation_get_size_value));

        scheme->steps.add(new operation_step(operation_create_object_array));
    }
}

void create_null_value_operation(operation_scheme *scheme) {
    if(scheme) {
        if(scheme->schemeType == scheme_none)
            scheme->schemeType = scheme_nullify_value;

        scheme->steps.add(new operation_step(
                operation_nullify_value));
    }
}

void create_instance_field_access_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->schemeType = scheme_access_instance_field;
        scheme->field = instanceField;

        scheme->steps.add(new operation_step(
                operation_get_instance_field_value, instanceField));
    }
}

void create_instance_field_getter_operation(
        operation_scheme *scheme,
        sharp_field *instanceField) {
    if(scheme) {
        scheme->schemeType = scheme_call_getter_function;
        scheme->fun = instanceField->getter;

        scheme->steps.add(new operation_step(
                operation_call_instance_function, instanceField->getter));
    }
}

void create_get_static_function_address_operation(
        operation_scheme *scheme,
        sharp_function *fun) {
    if(scheme) {
        scheme->free();
        scheme->schemeType = scheme_get_address;

        scheme->steps.add(new operation_step(
                operation_get_static_function_address, fun));
    }
}

void create_primary_class_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme> &paramScheme,
        sharp_function *fun) {
    if(scheme) {
        scheme->schemeType = scheme_call_instance_function;
        scheme->fun = fun;
        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, fun->owner
        ));

        scheme->steps.add(new operation_step(
                operation_push_value_to_stack
        ));

        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_scheme(paramScheme.get(i)),
                    operation_push_parameter_to_stack
                    ));
        }

        scheme->steps.add(new operation_step(
                operation_call_instance_function, fun));
    }
}

void create_instance_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme> &paramScheme,
        sharp_function *fun) {
    if(scheme) {
        scheme->schemeType = scheme_call_instance_function;
        scheme->fun = fun;
        scheme->steps.add(new operation_step(
                operation_push_value_to_stack
        ));

        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_scheme(paramScheme.get(i)),
                    operation_push_parameter_to_stack
            ));
        }

        scheme->steps.add(new operation_step(
                operation_call_instance_function, fun));
    }
}

void create_get_integer_constant_operation(
        operation_scheme *scheme,
        Int integer) {
    scheme->schemeType = scheme_get_constant;
    scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_integer_constant, integer));
}

void create_get_decimal_constant_operation(
        operation_scheme *scheme,
        long double decimal) {
    scheme->schemeType = scheme_get_constant;
    scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_decimal_constant, decimal));
}

void create_get_char_constant_operation(
        operation_scheme *scheme,
        char _char) {
    scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_char_constant, _char));
}

void create_get_bool_constant_operation(
        operation_scheme *scheme,
        bool _bool) {
    scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_bool_constant, _bool));
}

void create_get_string_constant_operation(
        operation_scheme *scheme,
        string &_string) {
    scheme->free();

    scheme->steps.add(new operation_step(
            operation_get_string_constant, _string));
}

void create_negate_operation(operation_scheme *scheme) {
    scheme->steps.add(new operation_step(operation_negate_value));
}
void create_not_operation(operation_scheme *scheme) {
    scheme->steps.add(new operation_step(operation_not_value));
}

void create_increment_operation(
        operation_scheme *scheme,
        native_type type) {
    if(type == type_var) {
        scheme->steps.add(new operation_step(operation_var_increment));
    } else if(type == type_int8) {
        scheme->steps.add(new operation_step(operation_int8_increment));
    } else if(type == type_int16) {
        scheme->steps.add(new operation_step(operation_int16_increment));
    } else if(type == type_int32) {
        scheme->steps.add(new operation_step(operation_int32_increment));
    } else if(type == type_int64) {
        scheme->steps.add(new operation_step(operation_int64_increment));
    } else if(type == type_uint8) {
        scheme->steps.add(new operation_step(operation_uint8_increment));
    } else if(type == type_uint16) {
        scheme->steps.add(new operation_step(operation_uint16_increment));
    } else if(type == type_uint32) {
        scheme->steps.add(new operation_step(operation_uint32_increment));
    } else if(type == type_uint64) {
        scheme->steps.add(new operation_step(operation_uint64_increment));
    } else {
        scheme->steps.add(new operation_step(operation_illegal));
    }
}

void create_get_primary_instance_class(
        operation_scheme *scheme,
        sharp_class *primaryClass) {
    if(scheme) {
        scheme->steps.add(new operation_step(
                operation_get_primary_class_instance, primaryClass));
    }
}

void create_decrement_operation(
        operation_scheme *scheme,
        native_type type) {
    if(type == type_var) {
        scheme->steps.add(new operation_step(operation_var_decrement));
    } else if(type == type_int8) {
        scheme->steps.add(new operation_step(operation_int8_decrement));
    } else if(type == type_int16) {
        scheme->steps.add(new operation_step(operation_int16_decrement));
    } else if(type == type_int32) {
        scheme->steps.add(new operation_step(operation_int32_decrement));
    } else if(type == type_int64) {
        scheme->steps.add(new operation_step(operation_int64_decrement));
    } else if(type == type_uint8) {
        scheme->steps.add(new operation_step(operation_uint8_decrement));
    } else if(type == type_uint16) {
        scheme->steps.add(new operation_step(operation_uint16_decrement));
    } else if(type == type_uint32) {
        scheme->steps.add(new operation_step(operation_uint32_decrement));
    } else if(type == type_uint64) {
        scheme->steps.add(new operation_step(operation_uint64_decrement));
    } else {
        scheme->steps.add(new operation_step(operation_illegal));
    }
}

void create_static_function_call_operation(
        operation_scheme *scheme,
        List<operation_scheme> &paramScheme,
        sharp_function *fun) {
    if(scheme) {
        scheme->schemeType = scheme_call_instance_function;
        scheme->fun = fun;
        scheme->free();

        for(Int i = 0; i < paramScheme.size(); i++) {
            scheme->steps.add(new operation_step(
                    new operation_scheme(paramScheme.get(i)),
                    operation_push_parameter_to_stack
            ));
        }

        scheme->steps.add(new operation_step(
                operation_call_static_function, fun));
    }
}


void create_function_parameter_push_operation(
        sharp_type *paramType,
        Int matchResult,
        sharp_function *constructor,
        operation_scheme *paramScheme,
        operation_scheme *resultScheme) {
    if(paramType && paramScheme && resultScheme) {
        if(matchResult == match_normal) {
            resultScheme->copy(*paramScheme);
        } else { // assuming matchResult is match_constructor
            sharp_class *with_class = paramType->_class;

            if(with_class) {
                resultScheme->schemeType = scheme_new_class;
                resultScheme->sc = with_class;

                resultScheme->steps.add(
                        new operation_step(
                            operation_create_class,
                            with_class
                        )
                );

                List<operation_scheme> scheme;
                scheme.add(*paramScheme);
                create_instance_function_call_operation(
                        resultScheme, scheme, constructor);
                scheme.free();
            }
        }
    }
}

void operation_step::freeStep()  {
    if(scheme) delete scheme;
}

void operation_scheme::copy(const operation_scheme &scheme) {
    schemeType = scheme.schemeType;
    field = scheme.field;
    fun = scheme.fun;
    sc = scheme.sc;
    for(Int i = 0; i < scheme.steps.size(); i++) {
        steps.add(new operation_step(*scheme.steps.get(i)));
    }
}
