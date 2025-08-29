//
// Created by bknun on 7/7/2023.
//

#include "get_value_analyzer.h"

code_fragment *analyze_get_value(operation_schema *scheme) {
    if(scheme->schemeType == scheme_get_value) {
        return require_non_null(analyze_code(scheme->steps[0]->scheme));
    }

    return NULL;
}

code_fragment *analyze_negate_value(operation_schema *scheme) {
    if(scheme->schemeType == scheme_negate_value) {
        if(scheme->steps.get(0)->type == operation_get_integer_constant
            || scheme->steps.get(0)->type == operation_get_bool_constant
            || scheme->steps.get(0)->type == operation_get_decimal_constant) {
            return new get_constant_value(scheme);
        } else {
            return new negated_value(scheme, require_non_null(analyze_code(scheme->steps[0]->scheme)));
        }
    }

    return NULL;
}

code_fragment *analyze_inc_value(operation_schema *scheme) {
    if(scheme->schemeType == scheme_inc_value) {
        if(scheme->steps.get(0)->type == operation_get_integer_constant
            || scheme->steps.get(0)->type == operation_get_bool_constant
            || scheme->steps.get(0)->type == operation_get_decimal_constant) {
            return new get_constant_value(scheme);
        } else {
            return new incremented_value(scheme, require_non_null(analyze_code(scheme->steps[0]->scheme)));
        }
    }

    return NULL;
}

code_fragment *analyze_dec_value(operation_schema *scheme) {
    if(scheme->schemeType == scheme_dec_value) {
        if(scheme->steps.get(0)->type == operation_get_integer_constant
            || scheme->steps.get(0)->type == operation_get_bool_constant
            || scheme->steps.get(0)->type == operation_get_decimal_constant) {
            return new get_constant_value(scheme);
        } else {
            return new decremented_value(scheme, require_non_null(analyze_code(scheme->steps[0]->scheme)));
        }
    }

    return NULL;
}

code_fragment *analyze_not_value(operation_schema *scheme) {
    if(scheme->schemeType == scheme_not_value) {
        if(scheme->steps.get(0)->type == operation_get_integer_constant
            || scheme->steps.get(0)->type == operation_get_bool_constant
            || scheme->steps.get(0)->type == operation_get_decimal_constant) {
            return new get_constant_value(scheme);
        } else {
            return new not_value(scheme, require_non_null(analyze_code(scheme->steps[0]->scheme)));
        }
    }

    return NULL;
}