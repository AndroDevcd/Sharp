//
// Created by BNunnally on 12/22/2021.
//

#include "and_expression.h"
#include "../expression.h"
#include "../../../../taskdelegator/task_delegator.h"
#include "../../../types/types.h"
#include "binary_expression.h"

void compile_and_expression(expression *e, Ast *ast) {
    expression left, right;
    Token &operand = ast->getToken(0);

    compile_expression(left, ast->getSubAst(0));
    compile_expression(right, ast->getSubAst(1));

    // `a && b` and `a || b` with a number object on either side (bool, int, ...
    // - e.g. a function declared `: bool`): test the number inside the object.
    //
    // Otherwise the object's operator&& / operator|| function was called, and
    // a function call needs both arguments first: the right side always ran
    // (`i < n && items[i].ok()` read past the end, `ready() || wait()` waited).
    // With plain numbers on both sides the generated code jumps over the right
    // side, as in C++. (& | ^ are untouched: they never short-circuit.)
    if(operand == "&&" || operand == "||") {
        extract_value_field_from_expression(left, "", ast->getSubAst(0));
        extract_value_field_from_expression(right, "", ast->getSubAst(1));
    }

    compile_binary_expression(e, ast, left, right, operand);
}
