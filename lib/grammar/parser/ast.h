//
// Created by bknun on 1/7/2017.
//

#ifndef SHARP_AST_H
#define SHARP_AST_H

#include <list>
#include "tokenizer/tokenentity.h"

enum ast_types
{
    ast_class_decl,
    ast_import_decl,
    ast_module_decl,
    ast_macros_decl,
    ast_method_decl,
    ast_construct_decl,
    ast_extern_method_decl,
    ast_extern_typeid_decl,
    ast_label_decl,
    ast_operator_decl,
    ast_var_decl,
    ast_value,
    ast_value_list,
    ast_utype_arg_list,
    ast_utype_arg_list_opt,
    ast_vector_array,
    ast_mem_access_flag,
    ast_utype_arg,
    ast_utype_arg_opt,
    ast_expression,
    ast_primary_expr,
    ast_dotnotation_call_expr,
    ast_utype,
    ast_type_arg,
    ast_type_declarator,
    ast_method_params,
    ast_method_inv_params,
    ast_block,
    ast_finally_block,
    ast_catch_clause,
    ast_method_return_type,
    ast_return_stmnt,
    ast_var_assign_stmnt,
    ast_statement,
    ast_if_statement,
    ast_elseif_statement,
    ast_else_statement,
    ast_trycatch_statement,
    ast_throw_statement,
    ast_continue_statement,
    ast_break_statement,
    ast_goto_statement,
    ast_while_statement,
    ast_type_identifier,
    ast_refrence_pointer,
    ast_modulename,

    ast_entity, // the base level ast
    ast_none
};

class ast
{
public:
    ast(ast* parent, ast_types type, int line, int col)
            :
            type(type),
            parent(parent),
            line(line),
            col(col),
            numEntities(0),
            numAsts(0)
    {
        sub_asts = new list<ast>();
        entities = new list<token_entity>();
    }

    ast_types gettype();
    ast* getparent();
    long getsubastcount();
    ast *getsubast(long at);
    bool hassubast(ast_types at);
    bool hasentity(token_type t);
    void freesubs();
    long getentitycount();
    token_entity getentity(long at);

    void add_entity(token_entity entity);
    void add_ast(ast _ast);
    void free();

    void freeentities();
    void freelastsub();
    void freelastentity();

    int line, col;
    long numEntities, numAsts;
private:
    ast_types type;
    ast *parent;
    list<ast> *sub_asts;
    list<token_entity> *entities;

};

#endif //SHARP_AST_H
