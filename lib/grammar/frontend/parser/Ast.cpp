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
    if(sub_asts.size() == 0 || at >= sub_asts.size()) {
        stringstream ss;
        ss << "internal error, ast not found at index " << at;
        throw runtime_error(ss.str());
    }
    return sub_asts.get(at);
}

long Ast::getEntityCount()
{
    return entities.size();
}

Token Ast::getEntity(long at)
{
    return entities.get(at);
}

void Ast::addToken(Token entity)
{
    numEntities++;
    entities.add(entity);
}

void Ast::addAst(Ast* _ast)
{
    numAsts++;
    sub_asts.__new() = _ast;
}

void Ast::addAstFirst(Ast* _ast)
{
    numAsts++;
    sub_asts.insert(0, _ast);
}

void Ast::free() {

    Ast* pAst;
    for(int64_t i = 0; i < this->sub_asts.size(); i++)
    {
        pAst = this->sub_asts.get(i);
        pAst->free();
        delete pAst;
    }

    numAsts = 0;
    numEntities = 0;
    this->type = ast_none;
    this->sub_asts.free();
    this->entities.free();
}

void Ast::freeSubAsts() {
    Ast* pAst;
    for(int64_t i = 0; i < this->sub_asts.size(); i++)
    {
        pAst = this->sub_asts.get(i);
        pAst->free();
        delete pAst;
    }

    numAsts = 0;
    this->sub_asts.free();
}

void Ast::freeLastSub() {
    Ast* pAst = this->sub_asts.last();
    pAst->free();
    delete pAst;
    numAsts--;
    this->sub_asts.pop_back();
}

void Ast::freeAst(ast_type t) {
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(sub_asts.get(i)->getType() == t) {
            sub_asts.get(i)->free();
            delete sub_asts.get(i);
            numAsts--;
            this->sub_asts.pop_back();
            return;
        }
    }
}

void Ast::freeEntities() {
    numEntities = 0;
    this->entities.free();
}

void Ast::freeLastEntity() {
    numEntities--;
    this->entities.pop_back();
}

bool Ast::hasSubAst(ast_type at) {
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(sub_asts.get(i)->getType() == at)
            return true;
    }
    return false;
}

bool Ast::hasEntity(token_type t) {

    for(unsigned int i = 0; i < entities.size(); i++) {
        if(entities.at(i).getType() == t)
            return true;
    }
    return false;
}

bool Ast::findEntity(string t) {

    for(unsigned int i = 0; i < entities.size(); i++) {
        if(entities.at(i).getToken() == t)
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

Token Ast::getEntity(token_type t) {
    for(unsigned int i = 0; i < entities.size(); i++) {
        if(entities.at(i).getType() == t)
            return entities.at(i);
    }
    return Token();
}

Ast *Ast::getNextSubAst(ast_type at) {
    bool found = false;
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(found)
            return sub_asts.get(i);
        if(sub_asts.get(i)->getType() == at) {
            found = true;
        }
    }
    return NULL;
}

// tODO: add param bool override (default true) to override the encapsulation
Ast* Ast::encapsulate(ast_type at) {

    Ast *branch = new Ast(at, this->line, this->col);

    for(long int i = 0; i < sub_asts.size()-1; i++) {
        branch->addAst(sub_asts.get(i));
    }

    for(long int i = 0; i < entities.size(); i++) {
        branch->addToken(entities.get(i));
    }

    sub_asts.free();
    entities.free();
    addAst(branch);
    return branch;
}

void Ast::setAstType(ast_type t) {
    type = t;
}

Ast *Ast::getLastSubAst() {
    if(sub_asts.size() == 0) {
        stringstream ss;
        ss << "internal error, ast not found at index 0";
        throw runtime_error(ss.str());
    }
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

        for(long i = 0; i < ast->entities.size(); i++) {
            addToken(ast->entities.get(i));
        }
    }
}
