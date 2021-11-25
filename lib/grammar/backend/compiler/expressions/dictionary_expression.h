//
// Created by BNunnally on 11/23/2021.
//

#ifndef SHARP_DICTIONARY_EXPRESSION_H
#define SHARP_DICTIONARY_EXPRESSION_H

#include "../../../frontend/parser/Ast.h"
#include "../,./../../../../util/KeyPair.h"

struct expression;
struct sharp_type;
struct sharp_class;

void compile_dictionary_expression(expression *e, Ast *ast);
bool compile_dict_type(Ast *ast, sharp_type *, sharp_type *);
void compile_dict_item(
        Ast *ast,
        sharp_type *key,
        sharp_type *value,
        bool inferType,
        List<KeyPair<expression*, expression*>> &items);
void compile_dict_pair_item(expression *e, bool inferType, sharp_type *pairType);

// we pull this function from dependency.cpp
extern sharp_class *create_generic_class(List<sharp_type> &genericTypes, sharp_class *genericBlueprint);

#endif //SHARP_DICTIONARY_EXPRESSION_H
