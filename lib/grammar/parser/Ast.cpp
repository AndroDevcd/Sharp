//
// Created by bknun on 1/21/2018.
//

#include "Ast.h"
#include "../runtime.h"


ast_types Ast::getType()
{
    return type;
}

Ast* Ast::getParent()
{
    return parent;
}

long Ast::getSubAstCount()
{
    return sub_asts.size();
}

Ast* Ast::getSubAst(long at)
{
    if(numAsts == 0 || at >= numAsts) {
        stringstream ss;
        ss << "internal error, ast not found at index " << at;
        throw runtime_error(ss.str());
    }
    return &sub_asts.get(at);
}

long Ast::getEntityCount()
{
    return numEntities;
}

token_entity Ast::getEntity(long at)
{
    if(numEntities == 0) return token_entity();
    return entities.get(at);
}

void Ast::addEntity(token_entity entity)
{
    numEntities++;
    entities.add(entity);
}

void Ast::addAst(Ast _ast)
{
    numAsts++;
    sub_asts.add(_ast);
}

void Ast::free() {

    Ast* pAst;
    for(int64_t i = 0; i < this->sub_asts.size(); i++)
    {
        pAst = &this->sub_asts.get(i);
        pAst->free();
    }

    numAsts = 0;
    numEntities = 0;
    this->type = ast_none;
    this->parent = NULL;
    this->sub_asts.free();
    this->entities.free();
}

void Ast::freeSubAsts() {
    Ast* pAst;
    for(int64_t i = 0; i < this->sub_asts.size(); i++)
    {
        pAst = &this->sub_asts.get(i);
        pAst->free();
    }

    numAsts = 0;
    this->sub_asts.free();
}

void Ast::freeLastSub() {
    Ast* pAst = &this->sub_asts.last();
    pAst->free();
    numAsts--;
    this->sub_asts.pop_back();
}

void Ast::freeEntities() {
    numEntities = 0;
    this->entities.free();
}

void Ast::freeLastEntity() {
    numEntities--;
    this->entities.pop_back();
}

bool Ast::hasSubAst(ast_types at) {
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(sub_asts.get(i).getType() == at)
            return true;
    }
    return false;
}

bool Ast::hasEntity(token_type t) {

    token_entity e;
    for(unsigned int i = 0; i < entities.size(); i++) {
        e = entities.at(i);
        if(e.getTokenType() == t)
            return true;
    }
    return false;
}

Ast *Ast::getSubAst(ast_types at) {
    Ast* pAst;
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        pAst = &sub_asts.get(i);
        if(pAst->getType() == at)
            return pAst;
    }
    return NULL;
}

token_entity Ast::getEntity(token_type t) {
    token_entity e;
    for(unsigned int i = 0; i < entities.size(); i++) {
        e = entities.at(i);
        if(e.getTokenType() == t)
            return e;
    }
    return token_entity();
}

Ast *Ast::getNextSubAst(ast_types at) {
    bool found = false;
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(found)
            return &sub_asts.get(i);
        if(sub_asts.get(i).getType() == at) {
            found = true;
        }
    }
    return NULL;
}

// tODO: add param bool override (default true) to override the encapsulation
void Ast::encapsulate(ast_types at) {

    addAst(Ast(this, at, this->line, this->col));
    Ast* encap = getSubAst(getSubAstCount()-1);

    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(sub_asts.get(i).type != at)
            encap->addAst(sub_asts.get(i));
    }

    for(unsigned int i = 0; i < entities.size(); i++) {
        encap->addEntity(entities.get(i));
    }

    readjust:
    for(unsigned int i = 0; i < sub_asts.size(); i++) {
        if(sub_asts.get(i).type != at) {
            sub_asts.remove(i);
            goto readjust;
        }
    }

    numAsts = 1;
    numEntities = 0;
    entities.free();
}

void Ast::setAstType(ast_types t) {
    type = t;
}