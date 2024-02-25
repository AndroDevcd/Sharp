//
// Created by bnunnally on 8/29/21.
//

#ifndef SHARP_JSON_PERSER_H
#define SHARP_JSON_PERSER_H

#include "../../../stdimports.h"
#include "../frontend/tokenizer/tokentype.h"
#include "../frontend/tokenizer/token.h"
#include "json_value.h"

class json_member;

json_value* parse_json(string filename);
Token& next_token();
Token& peek_token();
void expect_token(token_type, string);
json_value* parse_element();
json_value* parse_value();
json_array* parse__array();
json_object* parse_object();
json_member* parse_member();
void parse_err(string);

#endif //SHARP_JSON_PERSER_H
