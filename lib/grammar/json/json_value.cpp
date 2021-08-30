//
// Created by bnunnally on 8/29/21.
//

#include "json_value.h"
#include "json_array.h"
#include "json_object.h"

void json_value::setArrayValue(json_array *value) {
    type = jtype_array;

    if(arrayValue) delete arrayValue;
    arrayValue = value;
}

void json_value::setObjectValue(json_object *value) {
    type = jtype_object;

    if(objectValue) delete objectValue;
    objectValue = value;
}

json_value::json_value(const json_value &jv)
        :
        json_value()
{
    copy(jv);
}

void json_value::copy(const json_value &jv) {
    free();

    type = jv.type;
    intValue = jv.intValue;
    boolValue = jv.intValue;
    stringValue = jv.stringValue;

    if(jv.objectValue) objectValue = new json_object(*jv.objectValue);
    if(jv.arrayValue) arrayValue = new json_array(*jv.arrayValue);
}

void json_value::free() {
    if(arrayValue) delete arrayValue;
    if(objectValue) delete objectValue;
    type = jtype_invalid;
    intValue = 0;
    boolValue = false;
    stringValue.clear();
}


void json_value::toString(stringstream &str, uInt &tabCount) {
    tabCount++;
    switch(type) {
        case jtype_int: {
            str << intValue;
            break;
        }
        case jtype_string: {
            str << "\"" << stringValue << "\"";
            break;
        }
        case jtype_array: {
            if(arrayValue) arrayValue->toString(str, tabCount);
            break;
        }
        case jtype_bool: {
            str << (boolValue ? "true" : "false");
            break;
        }
        case jtype_null: {
            str << "null";
            break;
        }
        case jtype_object: {
            if(objectValue) objectValue->toString(str, tabCount);
            break;
        }
        case jtype_invalid: {
            str << "invalid";
            break;
        }
        default: {
            str << "undefined";
        }
    }
    tabCount--;
}
