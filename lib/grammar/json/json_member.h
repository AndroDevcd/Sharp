//
// Created by bnunnally on 8/29/21.
//

#ifndef SHARP_JSON_MEMBER_H
#define SHARP_JSON_MEMBER_H

#include "json_value.h"

class json_member {
public:
    json_member()
    :
        name(""),
        value()
    {}

    ~json_member() {
        name.clear();
        value->free();
        delete value; value = NULL;
    }

    json_type getType() {
        return value->getType();
    }

    void toString(stringstream &str, uInt &tabCount) {
        addTabs(str, tabCount);
        str << "\"" << name << "\" : ";
        value->toString(str, tabCount);
    }

    string& getName() {
        return name;
    }

    void setName(string &name) {
        this->name = name;
    }

    json_value* getValue() {
        return value;
    }

    void copy(const json_member &jm) {
        this->name = jm.name;
        this->value = new json_value();
        this->value->copy(*jm.value);
    }

    void setValue(json_value *value) {
        this->value = value;
    }

private:
    string name;
    json_value* value;
};

#endif //SHARP_JSON_MEMBER_H
