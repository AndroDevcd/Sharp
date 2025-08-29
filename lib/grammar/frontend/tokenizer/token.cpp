//
// Created by bknun on 11/13/2017.
//

#include "token.h"
#include "../../json/json.h"

token_id Token::getId()
{
    return id;
}

void Token::setId(token_id id)
{
    this->id = id;
}

string &Token::getValue()
{
    return tok;
}

int Token::getColumn()
{
    return col;
}

int Token::getLine()
{
    return line;
}

bool Token::isSingle()
{
    return id == SINGLE;
}

token_type Token::getType()
{
    return type;
}

void Token::importData(json_value *jv) {
    if(jv) {
        auto jo = jv->getJsonObject();

        if (jo) {
            line = (*jo)["line"]->getValue()->getIntValue();
            col = (*jo)["col"]->getValue()->getIntValue();
            id = (token_id) (*jo)["id"]->getValue()->getIntValue();
            type = (token_type) (*jo)["type"]->getValue()->getIntValue();
            tok = (*jo)["tok"]->getValue()->getStringValue();
        }
    }
}

json_value *Token::exportData() {
    json_value *jv = new json_value();
    json_object *jo = new json_object();

    jo->addMember("line");
    jo->addMember("col");
    jo->addMember("id");
    jo->addMember("type");
    jo->addMember("tok");

    (*jo)["line"]->setValue(new json_value(line));
    (*jo)["col"]->setValue(new json_value(col));
    (*jo)["id"]->setValue(new json_value((Int)id));
    (*jo)["type"]->setValue(new json_value((Int)type));
    (*jo)["tok"]->setValue(new json_value(tok));

    jv->setObjectValue(jo);
    return jv;
}
