//
// Created by BNunnally on 12/1/2021.
//

#include "assign_expression.h"
#include "expression.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../settings/settings.h"

/**
 * Is the left side of `=` an array element (`a[i] = v`)? Then the step that
 * reads the element is turned into the one that stores it, and this is its
 * position; -1 means an ordinary assignment.
 *
 * Only when the element itself is what gets assigned (the type of the left
 * side is still marked as an array element). In `items[0].weight = 6` the
 * element is merely on the way to a field: that is a field assignment. The old
 * search took any element access anywhere in the left side - and the FIRST one
 * - so that statement stored the 6 over items[0] and never touched the field
 * (and a null element raised nothing).
 */
static Int array_element_store_position(expression &left) {
    if(!left.type.arrayElement)
        return -1;

    for(Int i = (Int)left.scheme.steps.size() - 1; i >= 0; i--) {
        if(left.scheme.steps.get(i)->type == operation_get_array_element_at_index)
            return i;       // the last one: in `x[i].list[j] = v` it is `[j]`
    }
    return -1;
}

/**
 * `left = right` where the types only fit through a function of left's class
 * (`text = "abc"`: string.operator=(_int8[])).
 *
 * operator= is called ON the object that `left` holds. That cannot work when
 * there may be no object: an element of an object array starts out null, and
 * so does a variable of a nullable type (`text : string?`). For those two
 * kinds of target a new object is constructed from the value and stored -
 * when the class has a fitting constructor:
 *
 *      names[1] = "one";       ->   names[1] = new string("one");
 *      maybe = "x";            ->   maybe = new string("x");      (maybe : string?)
 *
 * It used to call operator= on null and throw nullptr_exception.
 * Returns false when this does not apply (operator= is used as before).
 */
static bool assign_by_construction(expression *e, expression &left, expression &right, Ast *ast) {
    sharp_type target = get_real_type(left.type);
    if(!(left.type.arrayElement || target.nullable) || target.isArray)
        return false;

    sharp_function *constructor = NULL;
    uInt match = is_implicit_type_match(left.type, right.type, constructor_only, constructor);
    if(match != match_constructor || constructor == NULL)
        return false;

    operation_schema created;
    {
        APPLY_TEMP_SCHEME_WITHOUT_INJECT(5,
            compile_initialization_call(ast, constructor, right, &scheme_5);
        )
        created.copy(scheme_5);
    }

    Int lastPos = array_element_store_position(left);
    if(lastPos != -1) {
        left.scheme.steps.at(lastPos)->type = operation_set_array_element_at_index;
        create_array_index_assignment_operation(&e->scheme, &left.scheme, &created);
    } else {
        create_value_assignment_operation(&e->scheme, &left.scheme, &created);
    }

    e->type.copy(left.type);
    return true;
}

void compile_assign_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    sharp_function *matchedFun = NULL;
    uInt match_result = is_explicit_type_match(left.type, right.type);

    if(operand == "=") {
        auto primaryFunction = get_primary_function(&current_context);

        if (left.type == type_field
            && left.type.field->getter != NULL && left.type.field->getter != primaryFunction
            && left.type.field->setter != primaryFunction) {

            Int lastPos = -1;
            for(Int i = 0; i < left.scheme.steps.size(); i++) {
                if(left.scheme.steps.get(i)->type == operation_get_field_value) {
                    lastPos = i;
                }
            }

            if(lastPos != -1) {
                auto getGetterStep = left.scheme.steps.get(lastPos);
                if(getGetterStep->scheme->schemeType == scheme_call_getter_function) {
                    if(getGetterStep->scheme->steps.size() == 3
                       && getGetterStep->scheme->steps.last(0)->type == operation_call_instance_function
                       && getGetterStep->scheme->steps.last(1)->type == operation_push_value_to_stack
                       && getGetterStep->scheme->steps.last(2)->type == operation_get_static_class_instance) {
                        create_static_field_access_operation(getGetterStep->scheme, left.type.field);
                    } else if(getGetterStep->scheme->steps.size() == 3
                              && getGetterStep->scheme->steps.last(0)->type == operation_call_instance_function
                              && getGetterStep->scheme->steps.last(1)->type == operation_push_value_to_stack
                              && getGetterStep->scheme->steps.last(2)->type == operation_get_primary_class_instance) {
                        create_primary_instance_field_access_operation(getGetterStep->scheme, left.type.field); // todo: inject these instructions instead
                    } else if(getGetterStep->scheme->steps.size() == 1
                              && left.scheme.steps.last(0)->type == operation_call_instance_function
                              && left.scheme.steps.last(0)->function == left.type.field->getter) {
                        getGetterStep->scheme->steps.free();
                        create_primary_instance_field_access_operation(getGetterStep->scheme, left.type.field);
                    }
                }
            }
        }

        if (left.type == type_field
            && left.type.field->setter != NULL && left.type.field->setter != primaryFunction
            && left.type.field->getter != primaryFunction) {

            Int lastPos = -1;
            for(Int i = 0; i < left.scheme.steps.size(); i++) {
                if(left.scheme.steps.get(i)->type == operation_get_field_value) {
                    lastPos = i;
                }
            }

            if(lastPos != -1) {
                auto getValueStep = left.scheme.steps.get(lastPos);
                if(getValueStep->scheme->steps.last()->type == operation_get_instance_field_value) {
                    getValueStep->scheme->steps.pop_back();
                    getValueStep->type = operation_get_value;
                } else if(getValueStep->scheme->steps.last()->type == operation_get_tls_field_value) {
                    left.scheme.steps.removeAt(lastPos);
                }
            }

            sharp_field *field = left.type.field;
            if (match_result == no_match_found) {
                match_result =
                        is_implicit_type_match(left.type, right.type,
                                               constructor_only, matchedFun);
            }

            if (match_result == no_match_found
                && (is_implicit_type_match(
                    left.type, right.type,
                    match_constructor | match_initializer,
                    matchedFun) != no_match_found)) {
                create_new_error(GENERIC, ast->line, ast->col,
                                 "Conflicting assignment on field `" + field->name +
                                 "` of type `" + type_to_str(field->type) +
                                 "`. Cannot assign value via setter when `operator=` function is required.");
            }

            string name = "";
            string stdModule = "std";
            if (match_result != no_match_found) {
                List<sharp_field *> params;
                List<operation_schema *> paramOperations;

                impl_location location;
                params.add(new sharp_field(
                        name, get_primary_class(&currThread->currTask->file->ctx), location,
                        right.type, flag_public, normal_field,
                        ast
                ));
                paramOperations.add(new operation_schema(right.scheme));

                e->scheme.copy(left.scheme);
                compile_function_call(
                        &e->scheme, params,
                        paramOperations, field->setter,
                        check_flag(field->setter->flags, flag_static),
                        false,
                        false);
            } else {
                create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                                 "cannot assign field `" + field->fullName + "` of type `" + type_to_str(field->type)
                                 + "` to expression of type `" + type_to_str(right.type) + "`.");
            }

            return;
        }

        if (get_class_type(left.type) != NULL || is_object_type(left.type)) {

            if (match_result == no_match_found) {
                match_result =
                        is_implicit_type_match(left.type, right.type,
                              overload_only, matchedFun);
            }

            if(is_match_normal(match_result)) {
                Int lastPos = array_element_store_position(left);

                if(lastPos != -1) {
                    left.scheme.steps.at(lastPos)->type = operation_set_array_element_at_index;
                    create_array_index_assignment_operation(&e->scheme, &left.scheme, &right.scheme);
                } else {
                    create_value_assignment_operation(&e->scheme, &left.scheme, &right.scheme);
                }

                e->type.copy(left.type);

            } else if(is_match_function(match_result) && assign_by_construction(e, left, right, ast)) {
                // done: `names[i] = "text"` / `maybe_text = "text"` made a new object
            } else if(is_match_function(match_result)){
                List<sharp_field *> params;
                List<operation_schema *> paramOperations;
                e->scheme.copy(left.scheme);
                e->type.copy(matchedFun->returnType);

                string name = "";
                string stdModule = "std";
                impl_location location;
                params.add(new sharp_field(
                        name, get_primary_class(&currThread->currTask->file->ctx), location,
                        right.type, flag_public, normal_field,
                        ast
                ));
                paramOperations.add(new operation_schema(right.scheme));

                compile_function_call(
                        &e->scheme, params,
                        paramOperations, matchedFun,
                        false,
                        false,
                        false);
            } else if(match_result == indirect_match_w_nullability_mismatch) {
                create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                          " expressions are not compatible, assigning null to non nullable type of `" + type_to_str(left.type) + "`.");
            } else {
                create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                  " expressions are not compatible, assigning `" + type_to_str(right.type)
                  + "` to expression of type `" + type_to_str(left.type) + "`.");
            }
        } else if(is_numeric_type(left.type)) {
            if(left.type.type == type_field || left.type.arrayElement) {
                if (match_result == no_match_found) {
                    match_result =
                            is_implicit_type_match(left.type, right.type,
                                  exclude_all, matchedFun);
                }

                if(is_match_normal(match_result)) {
                    // `total = count` with total: var, count: _int32 (and the
                    // other way round): the value becomes the kind of the target
                    convert_number_for(right, left.type);
                    Int lastPos = array_element_store_position(left);

                    if(lastPos != -1) {
                        left.scheme.steps.at(lastPos)->type = operation_set_array_element_at_index;
                        create_array_index_assignment_operation(&e->scheme, &left.scheme, &right.scheme);
                    } else {
                        create_value_assignment_operation(&e->scheme, &left.scheme, &right.scheme);
                    }

                    e->type.copy(left.type);
                } else {
                    create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                               " expressions are not compatible, assigning `" + type_to_str(right.type)
                               + "` to field of type `" + type_to_str(left.type) + "`.");
                }
            } else {
                create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                         " expressions are not compatible, assigning `" + type_to_str(right.type)
                         + "` to expression of type `" + type_to_str(left.type) + "`.");
            }
        }
    } else {
        if(get_class_type(left.type) != NULL) {
            List<sharp_field *> params;
            List<operation_schema *> operations;
            operations.add(new operation_schema());
            create_get_value_operation(operations.last(), &right.scheme, false);
            convert_expression_type_to_real_type(right);

            string name;
            sharp_type type;
            type.copy(right.type);
            impl_location location(current_file, ast);
            params.add(new sharp_field(name, get_primary_class(&current_file->ctx),
                                       location, type, flag_public, normal_field, ast));

            compile_class_function_overload(
                    get_class_type(left.type), left, params, operations, operand.getValue(), ast);
            e->type.copy(left.type);
            e->scheme.copy(left.scheme);
        } else if(is_numeric_type(left.type) && left.type.type != type_function_ptr) {
            // a field, or an element of a number array (`a[i] += v`; it used to
            // be rejected as "not assignable" - see
            // scheme_compound_assign_value.cpp for how it is stored)
            if(left.type == type_field || array_element_store_position(left) != -1) {
                if (match_result == no_match_found) {
                    match_result =
                            is_implicit_type_match(left.type, right.type,
                                                   exclude_all, matchedFun);
                }

                if(is_match_normal(match_result)) {
                    e->type.copy(left.type);

                    ALLOCATE_REGISTER_2X(0, 1, &e->scheme,
                        if(operand == "+=") {
                            create_plus_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "-=") {
                            create_sub_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "*=") {
                            create_mult_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "/=") {
                            create_div_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "%=") {
                            create_mod_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "&=") {
                            create_and_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "|=") {
                            create_or_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        } else if(operand == "^=") {
                            create_xor_value_assignment_operation(&e->scheme, register_0, register_1, &left.scheme, &right.scheme);
                        }

                        // "+=" -> "+": typed like the binary operator, result
                        // converted back to the kind of the variable
                        string arithmeticOperator = operand.getValue().substr(0, operand.getValue().size() - 1);
                        type_compound_assignment(&e->scheme, arithmeticOperator, left.type, right.type);
                    )
                } else {
                    create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                           "expressions are not compatible, assigning `" + type_to_str(right.type)
                           + "` to expression of type `" + type_to_str(left.type) + "`.");
                }
            } else {
                create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                        " expressions of type `" + type_to_str(left.type)
                        + "` is not assignable.");
            }
        } else {
            create_new_error(INCOMPATIBLE_TYPES, ast->line, ast->col,
                  "expressions of type `" + type_to_str(left.type)
                  + "` cannot be assigned via `" + operand.getValue() + "` operator.");
        }
    }
}
