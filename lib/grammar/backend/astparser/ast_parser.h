//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_AST_PARSER_H
#define SHARP_AST_PARSER_H

#include "../../frontend/parser/Ast.h"
#include "../access_flag.h"
#include "../types/unresolved_type.h"

struct sharp_class;

// This file is responsible for containing globally
// used functions for parsing data in an ast

string concat_tokens(Ast *ast);

uInt parse_access_flags(uInt allowedFlags, string memberType, sharp_class *memberOwner, Ast *ast);

access_flag str_to_access_flag(string&);

string access_flags_to_str(uInt);

void parse_utype(sharp_type &type, Ast *ast);
void parse_reference_pointer(sharp_type &sharpType, Ast *ast);

unresolved_item parse_reference_item(Ast *ast);
void parse_normal_reference_item(unresolved_item &item, Ast *ast);
void parse_utype_list(List<sharp_type> &types, Ast *ast);
void parse_utype_arg_list_opt(List<sharp_type> &types, Ast *ast);
void parse_type_identifier(sharp_type &type, Ast *ast);
void parse_function_pointer(sharp_type &type, Ast *ast);
void parse_function_return_type(sharp_type &type, Ast *ast);

#endif //SHARP_AST_PARSER_H
