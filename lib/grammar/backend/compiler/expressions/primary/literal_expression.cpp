//
// Created by BNunnally on 9/24/2021.
//

#include "literal_expression.h"
#include "../expression.h"
#include "../../../../settings/settings.h"
#include "../../../../compiler_info.h"
#include "../../../dependency/dependancy.h"
#include "../../../types/types.h"
#include "post_ast_expression.h"

/**
 * A string literal is an array of characters, not an object: "abc" by itself
 * has no functions. Before one is called on it the literal becomes
 * `new string("abc")` - the same object an assignment `s := "abc"` makes.
 */
void make_string_object(expression *e, Ast *ast) {
    string std = "std", name = "";
    sharp_class *stringClass = resolve_class(get_module(std), "string", false, false);
    if(stringClass == NULL) {
        create_new_error(GENERIC, ast, "the class `std#string` is needed to call a function on a string literal.");
        return;
    }

    List<sharp_field*> params;
    List<operation_schema*> paramOperations;
    impl_location location(current_file, ast);
    sharp_type literalType = get_real_type(e->type);
    literalType.nullable = true;
    params.add(new sharp_field(
            name, get_primary_class(&currThread->currTask->file->ctx), location,
            literalType, flag_public, normal_field, ast
    ));
    paramOperations.add(new operation_schema(e->scheme));

    sharp_function *constructor = resolve_function("string", stringClass, params,
                                                   constructor_function, exclude_all, ast, false, false);
    if(constructor == NULL) {
        create_new_error(GENERIC, ast, "no constructor of `std#string` takes a string literal.");
    } else {
        create_dependency(constructor);
        create_dependency(stringClass);

        operation_schema made;
        compile_initialization_call(ast, stringClass, constructor, params, paramOperations, &made);
        made.schemeType = scheme_new_class;

        e->scheme.free();
        e->scheme.copy(made);
        e->type.free();
        e->type.type = type_class;
        e->type._class = stringClass;
        e->type.isArray = false;        // the literal was an array of characters; this is one object
        e->type.nullable = false;
    }

    deleteList(params);
    deleteList(paramOperations);
}

void compile_literal_member_access(expression *e, Ast *literalExpression) {
    // sub tree 0 is the literal itself, what follows was written after it
    if(literalExpression->getSubAstCount() < 2 || e->type.type != type_string)
        return;

    make_string_object(e, literalExpression);
    compile_post_ast_expression(e, literalExpression, 1);
}

void compile_literal_expression(expression *e, Ast *ast) {
    Token &literal = ast->getToken(0);

    if(literal.getId() == CHAR_LITERAL)
        compile_char_literal(e, literal);
    else if(literal.getId() == INTEGER_LITERAL)
        compile_integer_literal(e, literal);
    else if(literal.getId() == HEX_LITERAL)
        compile_hex_literal(e, literal);
    else if(literal.getId() == STRING_LITERAL)
        compile_string_literal(e, literal);
    else
        compile_boolean_literal(e, literal);
}


void compile_char_literal(expression *e, Token &literal) {
    e->type.type = type_char;

    if(literal.getValue().size() == 1) {
        e->type._char = literal.getValue().at(0);
    } else {
        char escapedChar = literal.getValue().at(1);

        if(escapedChar == 'n')
            e->type._char = '\n';
        else if(escapedChar == 't')
          e->type._char = '\t';
        else if(escapedChar == 'b')
          e->type._char = '\b';
        else if(escapedChar == 'v')
          e->type._char = '\v';
        else if(escapedChar == 'r')
          e->type._char = '\r';
        else if(escapedChar == 'f')
            e->type._char = '\t';
        else if(escapedChar == '\\')
            e->type._char = '\\';
        else
            e->type._char = escapedChar;
    }
    create_get_char_constant_operation(&e->scheme, e->type._char);
}


string remove_underscores(string &str) {
    stringstream newstring;
    for(char c : str) {
        if(c != '_')
            newstring << c;
    }
    return newstring.str();
}

void compile_integer_literal(expression *e, Token &literal) {
    e->type.type = type_integer;
    string integerStr = remove_underscores(literal.getValue());

    if(isNumber(integerStr)) {
        e->type.integer
            = strtol(integerStr.c_str(), NULL, 0);
        create_get_integer_constant_operation(&e->scheme, e->type.integer);
    } else {
        e->type.type = type_decimal;
        e->type.decimal = strtold(integerStr.c_str(), NULL);
        create_get_decimal_constant_operation(&e->scheme, e->type.decimal);
    }
}

void compile_hex_literal(expression *e, Token &literal) {
    e->type.type = type_integer;
    string integerStr = remove_underscores(literal.getValue());

#if _ARCH_BITS == 64
    e->type.integer = strtoll(integerStr.c_str(), NULL, 16);
#else
    e->type.integer = strtol(integerStr.c_str(), NULL, 16);
#endif

    create_get_integer_constant_operation(&e->scheme, e->type.integer);
}

void compile_string_literal(expression *e, Token &literal) {
    e->type.type = type_string;
    e->type._string = literal.getValue();
    e->type.isArray = true;
    create_get_string_constant_operation(&e->scheme, e->type._string);
}

void compile_boolean_literal(expression *e, Token &literal) {
    e->type.type = type_bool;
    e->type._bool = literal.getValue() == "true";
    create_get_bool_constant_operation(&e->scheme, e->type._bool);
}
