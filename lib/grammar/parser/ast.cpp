//
// Created by bknun on 1/7/2017.
//
#include "ast.h"


ast_types ast::gettype()
{
    return type;
}

ast* ast::getparent()
{
    return parent;
}

long ast::getsubastcount()
{
    return sub_asts->size();
}

ast* ast::getsubast(long at)
{
    return &(*std::next(sub_asts->begin(), at));
}

long ast::getentitycount()
{
    return entities->size();
}

token_entity ast::getentity(long at)
{
    return *std::next(entities->begin(), at);
}

void ast::add_entity(token_entity entity)
{
    entities->push_back(entity);
}

void ast::add_ast(ast _ast)
{
    sub_asts->push_back(_ast);
}

void ast::free() {
    this->type = ast_none;
    this->parent = NULL;
    this->entities->clear();
    std::free(this->entities); this->entities = NULL;

    ast* pAst;
    for(int64_t i = 0; i < this->sub_asts->size(); i++)
    {
        pAst = &(*std::next(this->sub_asts->begin(),
                     i));
        pAst->free();
    }

    this->sub_asts->clear();
    std::free(this->sub_asts); this->sub_asts = NULL;
}

void ast::freesubs() {
    ast* pAst;
    for(int64_t i = 0; i < this->sub_asts->size(); i++)
    {
        pAst = &(*std::next(this->sub_asts->begin(),
                            i));
        pAst->free();
    }

    this->sub_asts->clear();
}
