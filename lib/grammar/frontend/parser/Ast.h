//
// Created by bknun on 1/21/2018.
//

#ifndef SHARP_AST_H
#define SHARP_AST_H

#include <list>
#include "../tokenizer/token.h"
#include "../../List.h"

enum ast_type
{
    ast_class_decl,
    ast_mutate_decl,
    ast_generic_class_decl,
    ast_generic_interface_decl,
    ast_interface_decl,
    ast_import_decl,
    ast_module_decl,
    ast_method_decl,
    ast_enum_decl,
    ast_init_decl,
    ast_delegate_decl,
    ast_construct_decl,
    ast_label_decl,
    ast_operator_decl,
    ast_variable_decl,
    ast_switch_declarator,
    ast_value,
    ast_getter,
    ast_setter,
    ast_value_list,
    ast_expression_list,
    ast_field_init_list,
    ast_utype_arg_list,
    ast_lambda_function,
    ast_field_init,
    ast_base_utype,
    ast_lambda_arg_list,
    ast_utype_arg_list_opt,
    ast_reference_pointer_list,
    ast_utype_list,
    ast_identifier_list,
    ast_enum_identifier_list,
    ast_vector_array,
    ast_dictionary_array,
    ast_dictionary_type,
    ast_utype_arg,
    ast_lambda_arg,
    ast_name,
    ast_utype_arg_opt,
    ast_expression,
    ast_array_expression,
    ast_primary_expr,
    ast_dotnotation_call_expr,
    ast_utype,
    ast_block,
    ast_switch_block,
    ast_finally_block,
    ast_assembly_block,
    ast_when_block,
    ast_when_clause,
    ast_when_else_clause,
    ast_catch_clause,
    ast_method_return_type,
    ast_return_stmnt,
    ast_statement,
    ast_if_statement,
    ast_switch_statement,
    ast_elseif_statement,
    ast_else_statement,
    ast_lock_statement,
    ast_trycatch_statement,
    ast_throw_statement,
    ast_continue_statement,
    ast_break_statement,
    ast_goto_statement,
    ast_when_statement,
    ast_while_statement,
    ast_do_while_statement,
    ast_assembly_statement,
    ast_alias_decl,
    ast_for_statement,
    ast_for_expresion_cond,
    ast_for_expresion_iter,
    ast_foreach_statement,
    ast_type_identifier,
    ast_enum_identifier,
    ast_func_ptr,
    ast_refrence_pointer,
    ast_modulename,
    ast_literal,
    ast_access_type,

    /**
     * Encapsulated ast's to make processing expressions easier
     */
    ast_literal_e,
    ast_utype_class_e,
    ast_dot_not_e,
    ast_self_e,
    ast_base_e,
    ast_null_e,
    ast_new_e,
    ast_not_e,
    ast_post_inc_e,
    ast_arry_e,
    ast_dot_fn_e,
    ast_cast_e,
    ast_pre_inc_e,
    ast_paren_e,
    ast_vect_e,
    ast_minus_e,
    ast_add_e,
    ast_mult_e,
    ast_shift_e,
    ast_less_e,
    ast_equal_e,
    ast_and_e,
    ast_ques_e,
    ast_assign_e,
    ast_sizeof_e,

    ast_none
};

class Ast {
public:
    Ast(ast_type type, int line, int col)
    :
        type(type),
        line(line),
        col(col)
    {
        sub_asts.init();
        tokens.init();
    }

    Ast()
            :
            type(ast_none),
            line(0),
            col(0)
    {
        sub_asts.init();
        tokens.init();
    }

    Ast(Ast *cpy)
            :
            type(ast_none),
            line(0),
            col(0)
    {
        sub_asts.init();
        tokens.init();
        copy(cpy);
    }

    Ast* encapsulate(ast_type at);

    ast_type getType();
    long getSubAstCount();
    Ast *getSubAst(long at);
    Ast *getLastSubAst();
    Ast *getSubAst(ast_type at);
    bool hasSubAst(ast_type at);
    bool hasToken(token_type t);
    bool hasToken(string s);
    void freeSubAsts();
    long getTokenCount();
    Token &getToken(long at);
    Token getToken(token_type t);
    static string astTypeToString(ast_type type);

    void addToken(Token entity);
    void addAst(Ast* _ast);
    void copy(Ast *ast);
    void free();

    void freeTokens();
    void freeLastToken();
    void freeLastSub();
    string toString();

    int line, col;

    void setAstType(ast_type types);

    List<Ast*> sub_asts;
private:
    ast_type type;
    List<Token> tokens;
};


#endif //SHARP_AST_H
