//
// Created by bknun on 7/23/2022.
//

#include "primary/literal_expression.h"
#include "inline_if_expression.h"
#include "expression.h"
#include "../../types/types.h"

void compile_inline_if_expression(expression *e, Ast *ast) {
    expression condExpr, trueExpr, falseExpr;

    compile_expression(condExpr, ast->getSubAst(0));
    compile_expression(trueExpr, ast->getSubAst(1));
    compile_expression(falseExpr, ast->getSubAst(2));

    // `flag ? "none" : name` - a literal on one side and a string object on
    // the other. The literal becomes a string object, so both sides are one
    // type. (It was an error: "expression of type string is not equal to that
    // of type string_literal".)
    {
        sharp_class *trueClass = get_class_type(trueExpr.type), *falseClass = get_class_type(falseExpr.type);
        if(trueExpr.type.type == type_string && falseClass != NULL && falseClass->fullName == "std#string")
            make_string_object(&trueExpr, ast->getSubAst(1));
        else if(falseExpr.type.type == type_string && trueClass != NULL && trueClass->fullName == "std#string")
            make_string_object(&falseExpr, ast->getSubAst(2));
    }

    extract_value_field_from_expression(condExpr, "", ast->getSubAst(0));
    convert_number_to_condition(condExpr);
    if(is_evaluable_type(condExpr.type)) {
        stringstream ss;
        sharp_label *endLabel, *ifFalseLabel;
        Int match_result = is_implicit_type_match(trueExpr.type, falseExpr.type, exclude_all);

        if(is_match_normal(match_result)) {
            // `flag ? 1 : 0.5` - the whole expression has the type of the first
            // branch, so the second branch is made that kind of number too
            convert_number_for(falseExpr, trueExpr.type);
            e->type.copy(trueExpr.type);

            set_internal_label_name(ss, "if_end", uniqueId++)
            endLabel = create_label(ss.str(), &current_context, ast, &e->scheme);
            set_internal_label_name(ss, "if_false", uniqueId++)
            ifFalseLabel = create_label(ss.str(), &current_context, ast, &e->scheme);

            create_get_value_operation(&e->scheme, &condExpr.scheme, false);
            create_jump_if_false_operation(&e->scheme, ifFalseLabel);
            create_get_value_operation(&e->scheme, &trueExpr.scheme, false);
            create_jump_operation(&e->scheme, endLabel);

            create_set_label_operation(&e->scheme, ifFalseLabel);
            create_get_value_operation(&e->scheme, &falseExpr.scheme, false);
            create_set_label_operation(&e->scheme, endLabel);

            e->scheme.schemeType = scheme_inline_if;
            e->scheme.type = get_real_type(trueExpr.type);
        } else if(match_result == indirect_match_w_nullability_mismatch) {
            create_new_error(NULLABILITY_MISMATCH, ast->line, ast->col, " expression of type `" + type_to_str(falseExpr.type) + "` is not equal to that of type `"
                                                                               + type_to_str(trueExpr.type) + "`.");
        } else {
            create_new_error(GENERIC, ast->line, ast->col, " expression of type `" + type_to_str(falseExpr.type) + "` is not equal to that of type `"
                                                                 + type_to_str(trueExpr.type) + "`, are you possibly missing a cast?");
        }
    } else {
        create_new_error(GENERIC, ast->line, ast->col, " condition expression of type `" +
                    type_to_str(condExpr.type) + "` must evaluate to true or false");
    }
}