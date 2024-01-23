//
// Created by bknun on 7/4/2023.
//

#ifndef SHARP_CODE_ANALYZER_H
#define SHARP_CODE_ANALYZER_H

#include "../../../../../../stdimports.h"
#include "../../../../compiler_info.h"
#include "../../../operation/operation.h"

enum fragment_type {
    no_fragment=0,
    code_block_fragment,
    while_block_fragment,
    if_block_fragment,
    data_nil,
    debug_info,
    variable_write,
    variable_read,
    new_class_fragment,
    get_constant,
    break_fragment,
    function_call_fragment,
    negated_value_fragment,
    incremented_value_fragment,
    decremented_value_fragment,
    unsupported_fragment, // officially unsupported fragments, this can be used as a quick way to traverse large code paths
};

struct code_fragment {
    code_fragment(fragment_type type)
    :
        type(type)
    {}

    fragment_type type;
};

struct unsupported: public code_fragment {
    unsupported(operation_schema *scheme)
    :
        code_fragment(unsupported_fragment),
        scheme(scheme),
        nodes()
    {}

    operation_schema *scheme;
    List<code_fragment*> nodes;
};

struct code_block: public code_fragment {
    code_block(operation_schema *scheme)
    :
        code_fragment(code_block_fragment),
        scheme(scheme),
        nodes()
    {}

    operation_schema *scheme;
    List<code_fragment*> nodes;
};

struct data_return: code_fragment {
    operation_schema *scheme;
    code_fragment *returnData;
};

struct nil_data: code_fragment {
    operation_schema *scheme;
};

struct debugging_info: code_fragment {
    debugging_info(operation_schema *scheme)
    :
        code_fragment(debug_info),
        scheme(scheme)
    {}

    operation_schema *scheme;
};

struct write_variable: code_fragment {
    write_variable(
            operation_schema *scheme,
            bool local,
            sharp_field *variable,
            code_fragment *assignment_data
    ):
        code_fragment(variable_write),
        scheme(scheme),
        isLocal(local),
        variable(variable),
        assignmentData(assignment_data)
    {}

    sharp_field *variable;
    bool isLocal;
    operation_schema *scheme;
    code_fragment *assignmentData;
};

struct new_class: code_fragment {
    new_class(
            operation_schema *scheme,
            sharp_class *_class,
            sharp_function *constr,
            List<code_fragment*> &params
    ):
        code_fragment(new_class_fragment),
        scheme(scheme),
        _class(_class),
        constructor(constr),
        params()
    {
        this->params.addAll(params);
    }

    sharp_class *_class;
    sharp_function *constructor;
    operation_schema *scheme;
    List<code_fragment*> params;
};

struct while_loop: public code_fragment {
    while_loop(
            operation_schema *scheme,
            code_fragment *cond,
            code_fragment *block
    ):
            code_fragment(while_block_fragment),
            scheme(scheme),
            conditionCheck(cond),
            block(block)
    {}

    operation_schema *scheme;
    code_fragment* conditionCheck;
    code_fragment* block;
};

struct if_blk: public code_fragment {
    if_blk(
            operation_schema *scheme,
            code_fragment *cond,
            code_fragment *block
    ):
            code_fragment(if_block_fragment),
            scheme(scheme),
            conditionCheck(cond),
            block(block)
    {}

    operation_schema *scheme;
    code_fragment* conditionCheck;
    code_fragment* block;
};

struct get_constant_value: code_fragment {
    get_constant_value(operation_schema *scheme)
            :
            code_fragment(get_constant),
            scheme(scheme)
    {}

    operation_schema *scheme;
};

struct break_loop: code_fragment {
    break_loop(operation_schema *scheme)
            :
            code_fragment(break_fragment),
            scheme(scheme)
    {}

    operation_schema *scheme;
};

struct function_call: code_fragment {
    function_call(operation_schema *scheme, List<code_fragment*> &actions)
            :
            code_fragment(function_call_fragment),
            scheme(scheme),
            actions()
    {
        this->actions.addAll(actions);
    }

    operation_schema *scheme;
    sharp_function *fun;
    List<code_fragment*> actions; // actions leading up to function call
};

struct read_variable: code_fragment {
    read_variable(
            operation_schema *scheme,
            bool local,
            sharp_field *variable,
            List<code_fragment*> &actions
    ):
            code_fragment(variable_read),
            scheme(scheme),
            isLocal(local),
            variable(variable),
            actions()
    {
        this->actions.addAll(actions);
    }

    sharp_field *variable;
    bool isLocal;
    operation_schema *scheme;
    List<code_fragment*> actions;
};

struct negated_value: public code_fragment {
    negated_value(
            operation_schema *scheme,
            code_fragment *value
    ):
            code_fragment(negated_value_fragment),
            scheme(scheme),
            value(value)
    {}

    operation_schema *scheme;
    code_fragment* value;
};

struct incremented_value: public code_fragment {
    incremented_value(
            operation_schema *scheme,
            code_fragment *value
    ):
            code_fragment(incremented_value_fragment),
            scheme(scheme),
            value(value)
    {}

    operation_schema *scheme;
    code_fragment* value;
};

struct decremented_value: public code_fragment {
    decremented_value(
            operation_schema *scheme,
            code_fragment *value
    ):
            code_fragment(decremented_value_fragment),
            scheme(scheme),
            value(value)
    {}

    operation_schema *scheme;
    code_fragment* value;
};

struct not_value: public code_fragment {
    not_value(
            operation_schema *scheme,
            code_fragment *value
    ):
            code_fragment(decremented_value_fragment),
            scheme(scheme),
            value(value)
    {}

    operation_schema *scheme;
    code_fragment* value;
};

code_fragment* analyze_code(operation_schema *scheme);
code_fragment* require_non_null(code_fragment *frag);
bool is_ignored_scheme(operation_schema *scheme);

#endif //SHARP_CODE_ANALYZER_H
