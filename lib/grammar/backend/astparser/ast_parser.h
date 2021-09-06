//
// Created by BNunnally on 8/31/2021.
//

#ifndef SHARP_AST_PARSER_H
#define SHARP_AST_PARSER_H

#include "../../frontend/parser/Ast.h"
#include "../access_flag.h"

struct sharp_class;

// This file is responsible for containing globally
// used functions for parsing data in an ast

string concat_tokens(Ast *ast);

uInt parse_access_flags(uInt allowedFlags, string memberType, sharp_class *memberOwner, Ast *ast);

access_flag str_to_access_flag(string&);

string access_flags_to_str(uInt);

#endif //SHARP_AST_PARSER_H
