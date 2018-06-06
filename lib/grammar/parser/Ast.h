//
// Created by bknun on 1/21/2018.
//

#ifndef SHARP_AST_H
#define SHARP_AST_H

#include <list>
#include "tokenizer/tokenentity.h"
#include "../List.h"

enum ast_types
{
    ast_class_decl,
    ast_generic_class_decl,
    ast_interface_decl,
    ast_import_decl,
    ast_module_decl,
    ast_method_decl,
    ast_delegate_post_decl,
    ast_delegate_decl,
    ast_construct_decl,
    ast_label_decl,
    ast_operator_decl,
    ast_var_decl,
    ast_value,
    ast_value_list,
    ast_utype_arg_list,
    ast_utype_arg_list_opt,
    ast_reference_identifier_list,
    ast_utype_list,
    ast_identifier_list,
    ast_vector_array,
    ast_utype_arg,
    ast_utype_arg_opt,
    ast_expression,
    ast_array_expression,
    ast_primary_expr,
    ast_dotnotation_call_expr,
    ast_utype,
    ast_block,
    ast_finally_block,
    ast_assembly_block,
    ast_catch_clause,
    ast_method_return_type,
    ast_return_stmnt,
    ast_statement,
    ast_if_statement,
    ast_elseif_statement,
    ast_else_statement,
    ast_lock_statement,
    ast_trycatch_statement,
    ast_throw_statement,
    ast_continue_statement,
    ast_break_statement,
    ast_goto_statement,
    ast_while_statement,
    ast_do_while_statement,
    ast_assembly_statement,
    ast_for_statement,
    ast_for_expresion_cond,
    ast_for_expresion_iter,
    ast_foreach_statement,
    ast_type_identifier,
    ast_func_prototype,
    ast_refrence_pointer,
    ast_modulename,
    ast_literal,

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
    Ast(Ast* parent, ast_types type, int line, int col)
    :
    type(type),
            parent(parent),
            line(line),
            col(col),
            numEntities(0),
    numAsts(0)
    {
        sub_asts.init();
        entities.init();
    }

    Ast()
            :
            type(ast_none),
            parent(NULL),
            line(0),
            col(0),
            numEntities(0),
            numAsts(0)
    {
        sub_asts.init();
        entities.init();
    }

    Ast(Ast* parent, Ast *cpy)
            :
            type(ast_none),
            parent(parent),
            line(0),
            col(0),
            numEntities(0),
            numAsts(0)
    {
        sub_asts.init();
        entities.init();
        copy(cpy);
    }

    void encapsulate(ast_types at);

    ast_types getType();
    Ast*  getParent();
    long getSubAstCount();
    Ast *getSubAst(long at);
    Ast *getLastSubAst();
    Ast *getSubAst(ast_types at);
    Ast *getNextSubAst(ast_types at);
    bool hasSubAst(ast_types at);
    bool hasEntity(token_type t);
    void freeSubAsts();
    long getEntityCount();
    token_entity getEntity(long at);
    token_entity getEntity(token_type t);

    void addEntity(token_entity entity);
    void addAst(Ast _ast);
    void copy(Ast *ast);
    void free();

    void freeEntities();
    void freeLastSub();
    void freeLastEntity();

    int line, col;
    long numEntities, numAsts;

    void setAstType(ast_types types);

private:
    ast_types type;
    Ast *parent;
    List<Ast> sub_asts;
    List<token_entity> entities;
};


#endif //SHARP_AST_H
