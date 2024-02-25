//
// Created by bnunnally on 8/29/21.
//

#include "json_parser.h"
#include "json_array.h"
#include "json_object.h"
#include "json_member.h"
#include "../frontend/tokenizer/tokenizer.h"
#include "../../util/File.h"
#include "../settings/settings.h"

string jsonData;
uInt pos = -1;
tokenizer *t;
List<Token> *_tokens;

#define tokens (*_tokens)
#define _tokenizer (*t)

json_value* parse_json(string filename) {
    pos = -1;

    if(!File::exists(filename.c_str())) {
        stringstream ss;
        ss << "json file: " << filename << " does not exist!";
        warning(ss.str());
        return NULL;
    }

    File::buffer buf;
    File::read_alltext(filename.c_str(), buf);
    jsonData = buf.to_str();
    buf.end();

    if(jsonData == "") {
        stringstream ss;
        ss << "json file: " << filename << " is empty";
        warning(ss.str());
        return NULL;
    }

    t = new tokenizer(jsonData, filename, false);
    if(!_tokenizer.getErrors()->hasErrors()) {
        _tokens = &_tokenizer.getTokens();

        try {
            return parse_element();
        } catch (runtime_error) { /* ignore and return null */ }
    }

    _tokenizer.getErrors()->printErrors();
    return NULL;
}

Token& next_token() {
    pos++;
    if(pos < tokens.size() && tokens[pos].getType() != _EOF)
        return tokens[pos];
    else {
        _tokenizer.getErrors()->createNewError(
                GENERIC, tokens[pos-1],
                "expected token before end of file"
                );
        throw runtime_error("");
    }
}

Token& peek_token() {
    if((pos+1) < tokens.size())
        return tokens[pos+1];
    else return tokens[pos];
}

void expect_token(token_type type, string tok) {
    Token &tk = next_token();
    if(!(tk.getType() == type && tk.getValue() == tok)) {
        stringstream ss;
        ss << "expected token: " << tok;
        parse_err(ss.str());
    }
}

json_value* parse_element() {
    return parse_value();
}

json_value* parse_value() {
    json_value *jv = new json_value();
    Token &tok = next_token();

    if(tok.getId() == IDENTIFIER) {
        if(tok.getValue() == "true" || tok.getValue() == "false") {
            jv->setBoolValue(tok.getValue() == "true");
        } else if(tok.getValue() == "null") {
            jv->setNullValue();
        } else {
            parse_err("expected 'true', 'false', or 'null'");
        }
    } else if(tok.getId() == INTEGER_LITERAL || tok.getId() == HEX_LITERAL) {
        jv->setIntValue(
             strtoll(
                     tok.getValue().c_str(),
                     nullptr,
                     tok.getId() == INTEGER_LITERAL ? 0 : 16
                   )
        );
    } else if(tok.getId() == STRING_LITERAL) {
        jv->setStringValue(tok.getValue());
    } else if(tok.getType() == LEFTBRACE) {
        jv->setArrayValue(parse__array());
    } else if(tok.getType() == LEFTCURLY) {
        jv->setObjectValue(parse_object());
    }

    return jv;
}

json_array* parse__array() {
    json_array *arry = new json_array();

    arry->addValue(parse_element());
    while(peek_token().getType() == COMMA) {
        expect_token(COMMA, ",");
        arry->addValue(parse_element());
    }

    expect_token(RIGHTBRACE, "]");
    return arry;
}

json_object* parse_object() {
    json_object *jo = new json_object();

    jo->addMember(parse_member());
    while(peek_token().getType() == COMMA) {
        expect_token(COMMA, ",");
        jo->addMember(parse_member());
    }

    expect_token(RIGHTCURLY, "}");
    return jo;
}

json_member* parse_member() {
    json_member *member = new json_member();

    if(peek_token().getId() == STRING_LITERAL) {
        member->setName(next_token().getValue());
        expect_token(COLON, ":");
        member->setValue(parse_element());
    } else if(peek_token().getId() == IDENTIFIER) {
        member->setName(next_token().getValue());
        expect_token(COLON, ":");
        member->setValue(parse_element());
    } else {
        parse_err("expected string literal or identifier for member declaration");
    }

    return member;
}

void parse_err(string err) {
    _tokenizer.getErrors()->createNewError(GENERIC, tokens[pos], err);

    throw runtime_error("");
}
