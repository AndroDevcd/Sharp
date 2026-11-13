//
// Created by BNunnally on 12/1/2021.
//

#include "assign_expression.h"
#include "expression.h"
#include "../../types/types.h"
#include "../../../taskdelegator/task_delegator.h"
#include "../../../settings/settings.h"
#include "binary/binary_expression.h"

void compile_assignment(expression *e, Ast *ast, expression &left, expression &right, Token &operand);

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

// ---------------------------------------------------------------------------------
// Properties: a field with `get` and `set`
//
//      class account {
//          balance : decimal = 0;
//              get { return balance; }
//              set v -> { balance = v; }
//      }
//
// From outside the class, reading `a.balance` calls the getter and
// `a.balance = 5` calls the setter. Changing it in place is both:
//
//      a.balance += 5;         a.balance = a.balance + 5;      getter, then setter
//      a.balance++;            a.balance = a.balance + 1;
//
// (These used to be compiled as if the field were a plain variable: += stopped
// the code generator and ++ changed a temporary copy.)
//
// C++ comparison: C++ has no properties; it is what `a.set_balance(a.balance() + 5)`
// does when written by hand.
// ---------------------------------------------------------------------------------

bool is_property_target(expression &left) {
    if(!(left.type == type_field)) return false;

    // inside its own getter / setter the field is the plain variable
    auto primaryFunction = get_primary_function(&current_context);
    sharp_field *field = left.type.field;
    return field->getter != NULL && field->setter != NULL
        && field->getter != primaryFunction && field->setter != primaryFunction;
}

/** `left = value`, where left is a property: the setter is called with `value` */
static void assign_property(expression *e, Ast *ast, expression &left, expression &value) {
    Token assign("=", ast->getToken(0).getId(), ast->getToken(0).getColumn(), ast->getToken(0).getLine());
    compile_assignment(e, ast, left, value, assign);
}

/** `left <op>= right` on a property, e.g. op = "+" for += */
static void compile_property_compound_assignment(
        expression *e, Ast *ast, expression &left, expression &right, Token &operand) {
    string arithmetic = operand.getValue().substr(0, operand.getValue().size() - 1);
    Token arithmeticToken(arithmetic, operand.getId(), operand.getColumn(), operand.getLine());

    // The left side is needed twice: once to read (getter) and once to write
    // (setter). The copy for the read gets label numbers of its own, because
    // a label can only be placed once.
    expression read(left), value;
    relabel_scheme(&read.scheme);
    compile_binary_expression(&value, ast, read, right, arithmeticToken);
    if(value.type == type_undefined)
        return;                         // the arithmetic was reported as an error

    assign_property(e, ast, left, value);
}

bool compile_property_step(expression *e, Ast *ast, const string &op) {
    if((op != "++" && op != "--") || !is_property_target(*e))
        return false;

    sharp_field *field = e->type.field;
    if(!(((field->type.type >= type_int8 && field->type.type <= type_uint64)
          || field->type.type == type_var) && !field->type.isArray))
        return false;                   // not a number: the usual rules (and errors) apply

    // the new value: what the getter gives, plus or minus one. (The copy for
    // the read gets its own label numbers, see the compound assignment.)
    expression target(*e), read(*e), value;
    relabel_scheme(&read.scheme);
    operation_schema *changed = new operation_schema();
    changed->schemeType = op == "++" ? scheme_inc_value : scheme_dec_value;
    create_get_value_operation(changed, &read.scheme, false, false);
    if(op == "++") create_increment_operation(changed, field->type.type);
    else create_decrement_operation(changed, field->type.type);
    value.scheme.copy(*changed);
    value.type.type = field->type.type;
    delete changed;

    e->scheme.free();
    assign_property(e, ast, target, value);
    return true;
}

void compile_assign_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);
    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    if(operand != "=" && is_property_target(left)) {
        compile_property_compound_assignment(e, ast, left, right, operand);
        return;
    }

    compile_assignment(e, ast, left, right, operand);
}

void compile_assignment(expression *e, Ast *ast, expression &left, expression &right, Token &operand) {
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
                    } else if(getGetterStep->scheme->steps.size() >= 2
                              && getGetterStep->scheme->steps.last(0)->type == operation_call_instance_function
                              && getGetterStep->scheme->steps.last(0)->function == left.type.field->getter
                              && getGetterStep->scheme->steps.last(1)->type == operation_push_value_to_stack) {
                        // `p.value = 10` on another object's property: the left
                        // side was built as "p, then call the getter". An
                        // assignment does not read the old value, so the getter
                        // call is taken out again and the field itself is named
                        // (the setter block below then calls the setter on `p`).
                        // It used to stay: the getter ran on every assignment,
                        // and the setter only found its object because the
                        // getter's result happened to sit in the same stack slot -
                        // which -O no longer guaranteed.
                        getGetterStep->scheme->steps.pop_back();
                        getGetterStep->scheme->steps.pop_back();
                        create_instance_field_access_operation(getGetterStep->scheme, left.type.field);
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
