//
// Created by bknun on 1/21/2018.
//

#include "Ast.h"


ast_type Ast::getType()
{
    return type;
}

long Ast::getSubAstCount()
{
    return sub_asts.size();
}

Ast* Ast::getSubAst(long at)
{
    return sub_asts.get(at);
}

long Ast::getTokenCount()
{
    return tokens.size();
}

Token &Ast::getToken(long at)
{
    return tokens.get(at);
}

void Ast::addToken(Token entity)
{
    tokens.add(entity);
}

void Ast::addAst(Ast* _ast)
{
    sub_asts.add(_ast);
}

void Ast::importData(json_value *jv) {
    if(jv) {
        auto jo = jv->getJsonObject();

        if (jo) {
            line = (*jo)["line"]->getValue()->getIntValue();
            col = (*jo)["col"]->getValue()->getIntValue();
            type = (ast_type) (*jo)["type"]->getValue()->getIntValue();

            if ((*jo)["tokens"]) {
                auto _tokens = (*jo)["tokens"]->getValue()
                        ->getArrayValue()->get_values();

                for (Int i = 0; i < _tokens.size(); i++) {
                    tokens.add(Token(_tokens.get(i)));
                }
            }

            if ((*jo)["children"]) {
                auto _children = (*jo)["children"]->getValue()
                        ->getArrayValue()->get_values();

                for (Int i = 0; i < _children.size(); i++) {
                    sub_asts.add(new Ast(_children.get(i)));
                }
            }
        }
    }
}

json_value* Ast::exportData() {
    json_value *jv = new json_value();
    json_object *jo = new json_object();

    jo->addMember("type");
    jo->addMember("line");
    jo->addMember("col");

    (*jo)["line"]->setValue(new json_value(line));
    (*jo)["col"]->setValue(new json_value(col));
    (*jo)["type"]->setValue(new json_value((Int)type));

    if(!tokens.empty()) {
        jo->addMember("tokens");
        json_value *tokenList = new json_value();
        json_array *tokenItems = new json_array();
        tokenList->setArrayValue(tokenItems);
        (*jo)["tokens"]->setValue(tokenList);

        for(Int i = 0; i < tokens.size(); i++) {
            tokenItems->addValue(tokens.get(i).exportData());
        }
    }

    if(!sub_asts.empty()) {
        jo->addMember("children");

        json_value *children = new json_value();
        json_array *childItems = new json_array();
        children->setArrayValue(childItems);
        (*jo)["children"]->setValue(children);
        for (Int i = 0; i < sub_asts.size(); i++) {
            childItems->addValue(sub_asts.get(i)->exportData());
        }
    }

    jv->setObjectValue(jo);
    return jv;
}

void Ast::free() {

    Ast* pAst;
    for(int64_t i = 0; i < this->sub_asts.size(); i++)
    {
        pAst = this->sub_asts.get(i);
        pAst->free();
        delete pAst;
    }

    this->type = ast_none;
    this->sub_asts.free();
    this->tokens.free();
}

void Ast::freeSubAsts() {
    Ast* pAst;
    for(int64_t i = 0; i < this->sub_asts.size(); i++)
    {
        pAst = this->sub_asts.get(i);
        pAst->free();
        delete pAst;
    }

    this->sub_asts.free();
}

void Ast::freeLastSub() {
    Ast* pAst = this->sub_asts.last();
    pAst->free();
    delete pAst;
    this->sub_asts.pop_back();
}

void Ast::freeTokens() {
    this->tokens.free();
}

void Ast::freeLastToken() {
    this->tokens.pop_back();
}

bool Ast::hasSubAst(ast_type at) {
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(sub_asts.get(i)->getType() == at)
            return true;
    }
    return false;
}

bool Ast::hasToken(token_type t) {

    for(unsigned int i = 0; i < tokens.size(); i++) {
        if(tokens.at(i).getType() == t)
            return true;
    }
    return false;
}

Ast *Ast::getSubAst(ast_type at) {
    Ast* pAst;
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        pAst = sub_asts.get(i);
        if(pAst->getType() == at)
            return pAst;
    }
    return NULL;
}

Token* Ast::getToken(token_type t) {
    for(unsigned int i = 0; i < tokens.size(); i++) {
        if(tokens.at(i).getType() == t)
            return &tokens.get(i);
    }
    return NULL;
}

Token* Ast::getToken(token_id t) {
    for(unsigned int i = 0; i < tokens.size(); i++) {
        if(tokens.at(i).getId() == t)
            return &tokens.get(i);
    }
    return NULL;
}

// tODO: add param bool override (default true) to override the encapsulation
Ast* Ast::encapsulate(ast_type at) {
    Ast *branch = new Ast(at, this->line, this->col);

    for(long int i = 0; i < sub_asts.size(); i++) {
        branch->addAst(sub_asts.get(i));
    }

    for(long int i = 0; i < tokens.size(); i++) {
        branch->addToken(tokens.get(i));
    }

    sub_asts.free();
    tokens.free();
    addAst(branch);
    return branch;
}

string Ast::astTypeToString(ast_type type) {
    switch (type) {
        case ast_class_decl:
            return "ast_class_decl";
        case ast_mutate_decl:
            return "ast_mutate_decl";
        case ast_generic_class_decl:
            return "ast_generic_class_decl";
        case ast_generic_interface_decl:
            return "ast_generic_interface_decl";
        case ast_interface_decl:
            return "ast_interface_decl";
        case ast_import_decl:
            return "ast_import_decl";
        case ast_module_decl:
            return "ast_module_decl";
        case ast_method_decl:
            return "ast_method_decl";
        case ast_enum_decl:
            return "ast_enum_decl";
        case ast_init_decl:
            return "ast_init_decl";
        case ast_minus_e:
            return "ast_minus_e";
        case ast_delegate_decl:
            return "ast_delegate_decl";
        case ast_construct_decl:
            return "ast_construct_decl";
        case ast_label_decl:
            return "ast_label_decl";
        case ast_operator_decl:
            return "ast_operator_decl";
        case ast_variable_decl:
            return "ast_variable_decl";
        case ast_switch_declarator:
            return "ast_switch_declarator";
        case ast_value:
            return "ast_value";
        case ast_value_list:
            return "ast_value_list";
        case ast_expression_list:
            return "ast_expression_list";
        case ast_field_init_list:
            return "ast_field_init_list";
        case ast_utype_arg_list:
            return "ast_utype_arg_list";
        case ast_lambda_function:
            return "ast_lambda_function";
        case ast_field_init:
            return "ast_field_init";
        case ast_base_utype:
            return "ast_base_utype";
        case ast_lambda_arg:
            return "ast_lambda_arg";
        case ast_lambda_arg_list:
            return "ast_lambda_arg_list";
        case ast_utype_arg_list_opt:
            return "ast_utype_arg_list_opt";
        case ast_reference_pointer_list:
            return "ast_reference_pointer_list";
        case ast_utype_list:
            return "ast_utype_list";
        case ast_identifier_list:
            return "ast_identifier_list";
        case ast_enum_identifier_list:
            return "ast_enum_identifier_list";
        case ast_getter:
            return "ast_getter";
        case ast_setter:
            return "ast_setter";
        case ast_vector_array:
            return "ast_vector_array";
        case ast_utype_arg:
            return "ast_utype_arg";
        case ast_name:
            return "ast_name";
        case ast_utype_arg_opt:
            return "ast_utype_arg_opt";
        case ast_expression:
            return "ast_expression";
        case ast_array_expression:
            return "ast_array_expression";
        case ast_primary_expr:
            return "ast_primary_expr";
        case ast_dotnotation_call_expr:
            return "ast_dotnotation_call_expr";
        case ast_utype:
            return "ast_utype";
        case ast_block:
            return "ast_block";
        case ast_switch_block:
            return "ast_switch_block";
        case ast_finally_block:
            return "ast_finally_block";
        case ast_assembly_block:
            return "ast_assembly_block";
        case ast_catch_clause:
            return "ast_catch_clause";
        case ast_method_return_type:
            return "ast_method_return_type";
        case ast_return_stmnt:
            return "ast_return_stmnt";
        case ast_statement:
            return "ast_statement";
        case ast_if_statement:
            return "ast_if_statement";
        case ast_elseif_statement:
            return "ast_elseif_statement";
        case ast_else_statement:
            return "ast_else_statement";
        case ast_lock_statement:
            return "ast_lock_statement";
        case ast_trycatch_statement:
            return "ast_trycatch_statement";
        case ast_throw_statement:
            return "ast_throw_statement";
        case ast_continue_statement:
            return "ast_continue_statement";
        case ast_break_statement:
            return "ast_break_statement";
        case ast_goto_statement:
            return "ast_goto_statement";
        case ast_while_statement:
            return "ast_while_statement";
        case ast_do_while_statement:
            return "ast_do_while_statement";
        case ast_assembly_statement:
            return "ast_assembly_statement";
        case ast_for_statement:
            return "ast_for_statement";
        case ast_for_expresion_cond:
            return "ast_for_expresion_cond";
        case ast_for_expresion_iter:
            return "ast_for_expresion_iter";
        case ast_foreach_statement:
            return "ast_foreach_statement";
        case ast_type_identifier:
            return "ast_type_identifier";
        case ast_enum_identifier:
            return "ast_enum_identifier";
        case ast_refrence_pointer:
            return "ast_refrence_pointer";
        case ast_modulename:
            return "ast_modulename";
        case ast_literal:
            return "ast_literal";
        case ast_access_type:
            return "ast_access_type";
        case ast_literal_e:
            return "ast_literal_e";
        case ast_utype_class_e:
            return "ast_utype_class_e";
        case ast_dot_not_e:
            return "ast_dot_not_e";
        case ast_self_e:
            return "ast_self_e";
        case ast_base_e:
            return "ast_base_e";
        case ast_null_e:
            return "ast_null_e";
        case ast_new_e:
            return "ast_new_e";
        case ast_not_e:
            return "ast_not_e";
        case ast_post_inc_e:
            return "ast_post_inc_e";
        case ast_arry_e:
            return "ast_arry_e";
        case ast_dot_fn_e:
            return "ast_dot_fn_e";
        case ast_cast_e:
            return "ast_cast_e";
        case ast_pre_inc_e:
            return "ast_pre_inc_e";
        case ast_paren_e:
            return "ast_paren_e";
        case ast_vect_e:
            return "ast_vect_e";
        case ast_add_e:
            return "ast_add_e";
        case ast_mult_e:
            return "ast_mult_e";
        case ast_shift_e:
            return "ast_shift_e";
        case ast_less_e:
            return "ast_less_e";
        case ast_equal_e:
            return "ast_equal_e";
        case ast_and_e:
            return "ast_and_e";
        case ast_ques_e:
            return "ast_ques_e";
        case ast_assign_e:
            return "ast_assign_e";
        case ast_sizeof_e:
            return "ast_sizeof_e";
        case ast_none:
            return "ast_none";
        case ast_dictionary_array:
            return "ast_dictionary_array";
        case ast_dictionary_type:
            return "ast_dictionary_type";
        case ast_dictionary_element:
            return "ast_dictionary_element";
        case ast_alias_decl:
            return "ast_alias_decl";
        case ast_func_ptr:
            return "ast_func_ptr";
        case ast_exponent_e:
            return "ast_exponent_e";
        case ast_obfuscate_decl:
            return "ast_obfuscate_decl";
        case ast_when_statement:
            return "ast_when_statement";
        case ast_when_block:
            return "ast_when_block";
        case ast_when_else_clause:
            return "ast_when_else_clause";
        case ast_when_clause:
            return "ast_when_clause";
    }

    return "unknown";
}

void Ast::setAstType(ast_type t) {
    type = t;
}

Ast *Ast::getLastSubAst() {
    return sub_asts.last();
}

void Ast::copy(Ast *ast) {
    free();
    if(ast != NULL) {
        this->line=ast->line;
        this->col=ast->col;
        this->type=ast->type;

        for(long i = 0; i < ast->sub_asts.size(); i++) {
            sub_asts.__new();
            sub_asts.last()->copy(ast->sub_asts.get(i));
        }

        for(long i = 0; i < ast->tokens.size(); i++) {
            addToken(ast->tokens.get(i));
        }
    }
}

bool Ast::hasToken(string s) {
    for(long i = 0; i < tokens.size(); i++) {
        if(tokens.get(i) == s)
            return true;
    }
    return false;
}

long indent = -1;
string getIndent() {
    stringstream str;
    for(long i = 0; i < indent; i++) {
        str << "|\t";
    }
    return str.str();
}

string Ast::toString() {
    stringstream astStr;
    indent++;
    astStr << getIndent() << "Ast (" << astTypeToString(type) << ")\n";
    astStr << getIndent() << "[\n";
    astStr << getIndent() << " tokens: (";
    for(long i = 0; i < tokens.size(); i++) {
        astStr << "\"" << tokens.get(i).getValue() + "\"";

        if((i + 1) < tokens.size()) {
            astStr << ", ";
        }
    }
    astStr << ")\n";

    astStr << getIndent() << " ast's: (" << sub_asts.size() << ")\n";
    for(long i = 0; i < sub_asts.size(); i++) {
        astStr << sub_asts.get(i)->toString();
    }
    astStr << "\n" << getIndent() << "]\n";
    indent--;
    return astStr.str();
}
